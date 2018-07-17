#include "block.h"
#include <unistd.h>

/* ASYNC ver */
int8_t target_lockflag;

Queue_t* FreeQ; // Queue of FREE(ERASE) state blocks

int32_t numLoaded;
int32_t numLoaded2;

struct algorithm __block={
	.create=block_create,
	.destroy=block_destroy,
	.get=block_get,
	.set=block_set,
	.remove=block_remove
};

/* Set set_pointer to first-meet ERASE index from current set_pointer like a round-robin */
int32_t block_findsp(int32_t checker){
	//if (set_pointer > 0)
		//set_pointer--;
	set_pointer = 0;

	for (; set_pointer < __block.li->NOB; ++set_pointer) {
		if (block_valid_array[set_pointer] == ERASE) {
			checker = 1;
			break;
		}
	}
	if (checker == 0) {
		for (set_pointer =0; set_pointer < __block.li->NOB; ++set_pointer)
			if (block_valid_array[set_pointer] == ERASE){
				checker = 1;
				break;
			}
	}
	return checker;
}

uint32_t block_create (lower_info* li,algorithm *algo){
	printf("block_create start!\n");
	algo->li=li;
	BM_Init();

	block_maptable = (int32_t*)malloc(sizeof(int32_t) * li->NOB);
	uint32_t i=0;
	for (; i<li->NOB; ++i){ // maptable initialization
		block_maptable[i] = NIL;
	}

	BM_validate_all(blockArray); // Actually, BM initialization is validate.


	block_valid_array = (int8_t*)malloc(sizeof(int8_t)*li->NOB);
	for (i = 0; i < li->NOB; ++i)
		block_valid_array[i] = ERASE; // 0 means ERASED, 1 means VALID

	//sram_valueset = (value_set*)malloc(sizeof(value_set) * _PPB);
	//for (int i=0; i<_PPB; i++)
		//sram_valueset + i = NULL;

	pthread_mutex_init(&lock, NULL);

	/* Queue */
	FreeQ = (Queue_t*)malloc(sizeof(Queue_t));
	InitQueue(FreeQ);
	for (int i=0; i<_NOB; ++i) {
		Enqueue(FreeQ, i);
	}
	printf("block_create end!\n");

	return 0;

}
void block_destroy (lower_info* li, algorithm *algo){

	free(block_maptable);
	free(block_valid_array);
	//free(sram_valueset);

	free(FreeQ);

	BM_Shutdown();
}
uint32_t block_get(request *const req){
	bench_algo_start(req);

	/* Request production */
	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req=block_end_req;

	uint32_t LBA = my_req->parents->key / __block.li->PPB;
	uint32_t offset = my_req->parents->key % __block.li->PPB;

	if (block_maptable[LBA] == NIL){
		printf("\nRead Empty Page..\n");
		bench_algo_end(req);
		return 0;
	}

	uint32_t PBA = block_maptable[LBA];
	uint32_t PPA = PBA * __block.li->PPB + offset;

	bench_algo_end(req);
	__block.li->pull_data(PPA, PAGESIZE, req->value, ASYNC, my_req);

	return 0;
}

uint32_t block_set(request *const req){
	algo_req* my_req;
	bench_algo_start(req);

	/* Request production */
	my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = block_end_req;
	if (my_req->parents->key % 1000 == 0)
		printf("Start set! key: %d\n", my_req->parents->key);

	//printf("seq:%d\n",set_seq++);
	//printf("block_set 1!\n");
	uint32_t LBA = my_req->parents->key / __block.li->PPB;
	uint32_t offset = my_req->parents->key % __block.li->PPB;
	uint32_t PBA;
	uint32_t PPA;
	int8_t checker = 0;


	//if (checker == 0) {
		/* There is NO free space in flash block */
		/* We need OverProvisioning area, maybe. */
#ifdef BFTL_DEBUG1	
	printf("LBA: %d, offset: %d\n", LBA, offset);
#endif

	if (block_maptable[LBA] == NIL)
	{
#ifdef BFTL_DEBUG1	
		printf("\tcase 1\n");
#endif
		checker = block_findsp(checker);
		//set_pointer = Dequeue(FreeQ);
		if (set_pointer == (uint32_t)-1) while(1) printf("WHAT!");

		// Switch ERASE to EXIST of block_valid_array
		block_valid_array[set_pointer] = EXIST;

		// Write PBA of mapping result in maptable
		block_maptable[LBA] = set_pointer;

		//PBA = block_maptable[LBA];
		//PPA = PBA * PPB + offset;
		PPA = set_pointer * _PPB + offset; // Equal to above 2 lines

		if (!BM_is_valid_ppa(blockArray, PPA))
			while(1)
				printf("ERROROROROROR!\n");
		BM_invalidate_ppa(blockArray, PPA);

		// write
		//printf("block_set 2!\n");
		//printf("my_req -> length: %d\n", my_req->parents->value->length);
		bench_algo_end(req);
		__block.li->push_data(PPA, PAGESIZE, req->value, ASYNC, my_req);
		//printf("block_set 3!\n");
	}

	else
	{
		PBA = block_maptable[LBA];
		PPA = PBA  * __block.li->PPB + offset;


		// data가 차있는 page를 valid라고 할 지 invalid라고 할 지 제대로 골라야 할 것 같다. 지금은 차있는 page를 invalid라고 설정하여 그 page에 write하려고 할 시에 GC를 하도록 하였다. valid로 바꾸는 게 맞는 것 같긴 한데, 그렇게 하면 write할 수 있는(비어있는) page들은 모두 invalid 상태라고 여기는 것이 된다. 그건 또 이상하다.. 그냥 block ftl에서는 write할 수 있는(비어있는) page를 valid page로, write할 수 없는(쓰여있는) page를 invalid page로 여기고 BM을 사용한다고 여기면 될 것 같다.
		// 그리고 block level에서의 valid/invalid 여부는 BM valid/invalid와 상관없다. 각 page마다 0/1을 구분하기 위해 BM의 page 단위 validity를 쓴 것일 뿐이다.
		if (BM_is_valid_ppa(blockArray, PPA))
		{
#ifdef BFTL_DEBUG1	
			printf("\tcase 2\n"); // 비어있는 page인 경우. 그대로 write 가능하다.
#endif
			int offsetcase = BM_CheckLastOffset(blockArray, PBA, offset);
			if (offsetcase == 0){
				// Moving Block with target offset update
				// 새로운 block으로 옮기고 metadata 다 update하기. 사실상 그냥 case 3이랑 똑같음
				GC_moving(req, my_req, LBA, offset, PBA, PPA, checker);
			}
			else {
				BM_invalidate_ppa(blockArray, PPA); // write되어 차있는 page라고 적어놓는다.
				block_valid_array[PBA] = EXIST;
				bench_algo_end(req);
				__block.li->push_data(PPA, PAGESIZE, req->value, ASYNC, my_req);
			}
		}
		else if (!BM_is_valid_ppa(blockArray, PPA))
		{
#ifdef BFTL_DEBUG1	
			printf("\tcase 3(GC)\n");
#endif
			GC_moving(req, my_req, LBA, offset, PBA, PPA, checker);
#ifdef ABC
			// Cleaning
			// Maptable update for data moving
#if 0
			printf("Start GC!\n");
			printf("offset: %d\n", offset);
			printf("PBA: %d\n", PBA);
			printf("PPA: %d\n", PPA);
			//exit(1);
#endif

			//checker = block_findsp(checker);
#if 0
				algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
				value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
				temp_req->parents = NULL;
				temp_req->end_req = block_algo_end_req;
				__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
				printf("numLoaded: %d\n", numLoaded);
				temp_set[i] = temp_value_set;
#endif
			//set_pointer = Dequeue(FreeQ);
			checker = block_findsp(checker);
			//printf("(3)Dequeue target: %d, old PBA: %d\n", set_pointer, PBA);
			//sleep(1);

			block_maptable[LBA] = set_pointer;
			block_valid_array[set_pointer] = EXIST;
			block_valid_array[PBA] = ERASE; // PBA means old_PBA

			uint32_t old_PPA_zero = PBA * _PPB;
			uint32_t new_PBA = block_maptable[LBA];
			uint32_t new_PPA_zero = new_PBA * _PPB;
			//uint32_t new_PPA = new_PBA * _PPB + offset;

			// Data move to new block
			uint32_t i = 0;

			/* Followings: ASC consideartion */

			/* Start move */
#ifdef BFTL_DEBUG1
			printf("Start move!\n");
#endif
			
			numLoaded = 0;
			value_set** temp_set = (value_set**)malloc(sizeof(value_set*)*_PPB);
			sram_valueset = (value_set*)malloc(sizeof(value_set) * _PPB);
			memset(sram_valueset, 0, sizeof(value_set) * _PPB);
			//sleep(2);

			for (i=0; i<_PPB; ++i) { // non-empty page만 옮겨야 하지 않을까? 이것도 상관은 없지만..
				// SRAM_load
				if (i != offset) {
					temp_set[i] = SRAM_load(i, old_PPA_zero);
#ifdef BFTL_DEBUG2
					printf("temp_set[%d]->value: %c\n", i, *(temp_set[i]->value));
#endif
				}
				else
					numLoaded++;
			}
			//printf("numLoaded: %d\n", numLoaded);
			
			while (numLoaded != _PPB) {printf("polling");} // polling for reading all pages in a block

			for (i=0; i<_PPB; ++i) {
				// copy data 
				if (i != offset) {
					memcpy(&sram_valueset[i], temp_set[i], sizeof(value_set));
				/*
				else {
					memcpy(&sram_valueset[i], req->value, sizeof(value_set)); // req->value: value_set*
					printf("target page offset: %d\n", i);
				}
				*/
#ifdef BFTL_DEBUG2
				printf("sram_valueset[%d].value: %c\n", i, *(sram_valueset[i].value));
#endif
				//printf("sram_valueset[%d].length: %u\n", i, (sram_valueset[i].length));
				}
			}


			for (i=0; i<_PPB; ++i) {
				// SRAM_unload
				BM_invalidate_ppa(blockArray, new_PPA_zero + i);
				BM_validate_ppa(blockArray, old_PPA_zero + i);
				if (i != offset)
					SRAM_unload(i, new_PPA_zero);
				else
					__block.li->push_data(new_PPA_zero + offset, PAGESIZE, req->value, ASYNC, my_req);

			}
			for (i=0; i<_PPB; ++i)
				if (i != offset)
					inf_free_valueset(temp_set[i], FS_MALLOC_R);

			//printf("numLoaded: %d\n", numLoaded);
			/* Trim the block of old PPA */
#ifdef BFTL_DEBUG1
			printf("trim!\n");
#endif
			__block.li->trim_block(old_PPA_zero, false);
			//Enqueue(FreeQ, PBA); // trim 직후에 하는 게 더 시기상 정확하긴 하다.
#ifdef BFTL_DEBUG1
			printf("trim end!\n");
#endif
			free(sram_valueset);
#endif // ! ABC
		}
	}
#ifdef BFTL_DEBUG1
	printf("end)LBA: %d, offset: %d\n", LBA, offset);
#endif

	return 0;

}
uint32_t block_remove(request *const req){
	//	block->li->trim_block()

	return 0;
}

void* block_end_req(algo_req* input){
	//block_params* params=(block_params*)input->params;

	//request *res=params->parents;
	request* res = input->parents;
	res->end_req(res);

	//free(params);
	free(input);
	if (target_lockflag)
		pthread_mutex_unlock(&lock);

	return NULL;
}


void* block_algo_end_req(algo_req* input){
	free(input);
	//pthread_mutex_unlock(&lock);
	numLoaded++; // pull할 때만 하도록 변경하기. 안해도 상관은 없음
	return NULL;
}

value_set* SRAM_load(uint32_t i, uint32_t old_PPA_zero)
{
	algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	temp_req->parents = NULL;
	temp_req->end_req = block_algo_end_req;
	__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
	//printf("numLoaded: %d\n", numLoaded);
	return temp_value_set;
}

void SRAM_unload(uint32_t i, uint32_t new_PPA_zero)
{
	algo_req* temp_req2 = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set2 = inf_get_valueset(sram_valueset[i].value, FS_MALLOC_W, PAGESIZE);
	temp_req2->parents = NULL;
	temp_req2->end_req = block_algo_end_req;
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, temp_req2);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
}

void GC_moving(request *const req, algo_req* my_req, uint32_t LBA, uint32_t offset, uint32_t PBA, uint32_t PPA, int8_t checker)
{
	checker = block_findsp(checker);
	block_maptable[LBA] = set_pointer;
	block_valid_array[set_pointer] = EXIST;
	block_valid_array[PBA] = ERASE; // PBA means old_PBA

	uint32_t old_PPA_zero = PBA * _PPB;
	uint32_t new_PBA = block_maptable[LBA];
	uint32_t new_PPA_zero = new_PBA * _PPB;
	uint32_t i = 0;

	/* Start move */
#ifdef BFTL_DEBUG1
	printf("Start move!\n");
#endif

	numLoaded = 0;
	value_set** temp_set = (value_set**)malloc(sizeof(value_set*)*_PPB);
	sram_valueset = (value_set*)malloc(sizeof(value_set) * _PPB);
	memset(sram_valueset, 0, sizeof(value_set) * _PPB);
	//sleep(2);

	for (i=0; i<_PPB; ++i) { // non-empty page만 옮겨야 하지 않을까? 이것도 상관은 없지만..
		// SRAM_load
		if (i != offset) {
			temp_set[i] = SRAM_load(i, old_PPA_zero);
#ifdef BFTL_DEBUG2
			printf("temp_set[%d]->value: %c\n", i, *(temp_set[i]->value));
#endif
		}
		else
			numLoaded++;
	}
	//printf("numLoaded: %d\n", numLoaded);

	while (numLoaded != _PPB) {printf("polling");} // polling for reading all pages in a block

	for (i=0; i<_PPB; ++i) {
		// copy data 
		if (i != offset) {
			memcpy(&sram_valueset[i], temp_set[i], sizeof(value_set));
			/*
			   else {
			   memcpy(&sram_valueset[i], req->value, sizeof(value_set)); // req->value: value_set*
			   printf("target page offset: %d\n", i);
			   }
			 */
#ifdef BFTL_DEBUG2
			printf("sram_valueset[%d].value: %c\n", i, *(sram_valueset[i].value));
#endif
			//printf("sram_valueset[%d].length: %u\n", i, (sram_valueset[i].length));
		}
	}


	for (i=0; i<_PPB; ++i) {
		// SRAM_unload
		BM_invalidate_ppa(blockArray, new_PPA_zero + i);
		BM_validate_ppa(blockArray, old_PPA_zero + i);
		if (i != offset)
			SRAM_unload(i, new_PPA_zero);
		else
			__block.li->push_data(new_PPA_zero + offset, PAGESIZE, req->value, ASYNC, my_req);

	}
	for (i=0; i<_PPB; ++i)
		if (i != offset)
			inf_free_valueset(temp_set[i], FS_MALLOC_R);

	//printf("numLoaded: %d\n", numLoaded);
	/* Trim the block of old PPA */
#ifdef BFTL_DEBUG1
	printf("trim!\n");
#endif
	__block.li->trim_block(old_PPA_zero, false);
	//Enqueue(FreeQ, PBA); // trim 직후에 하는 게 더 시기상 정확하긴 하다.
#ifdef BFTL_DEBUG1
	printf("trim end!\n");
#endif
	free(sram_valueset);
}
