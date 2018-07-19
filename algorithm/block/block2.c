#include "block.h"
#include <unistd.h>

/* ASYNC ver */

#ifdef Queue
Queue_t* FreeQ; // Queue of FREE(ERASE) state blocks
#endif

uint32_t numLoaded;
uint32_t nob;
uint32_t ppb_;
Block* blockArray;


struct algorithm __block={
	.create=block_create,
	.destroy=block_destroy,
	.get=block_get,
	.set=block_set,
	.remove=block_remove
};

/* Set set_pointer to first-meet ERASE index from current set_pointer like a round-robin */
int32_t block_findsp(int32_t checker)
{
	set_pointer = 0;

	for (; set_pointer < nob; ++set_pointer) {
		if (block_valid_array[set_pointer] == ERASE) {
			checker = 1;
			break;
		}
	}
	if (checker == 0) {
		for (set_pointer =0; set_pointer < nob; ++set_pointer)
			if (block_valid_array[set_pointer] == ERASE){
				checker = 1;
				break;
			}
	}
	return checker;
}

/* Check Last offset */
int8_t block_CheckLastOffset(uint32_t* lastoffset_array, uint32_t PBA, uint32_t offset)
{
	if (lastoffset_array[PBA] <= offset) {
		lastoffset_array[PBA] = offset;
		return 1;
	}
	else // Moving Block with target offset update
		return 0;
}

uint32_t block_create (lower_info* li,algorithm *algo){
	printf("block_create start!\n");
	algo->li=li;
	BM_Init(&blockArray);
	nob = _NOS;
	ppb_ = _PPS;

	block_maptable = (int32_t*)malloc(sizeof(int32_t) * nob);
	uint32_t i=0;
	for (; i<nob; ++i){ // maptable initialization
		block_maptable[i] = NIL;
	}

	BM_ValidateAll(blockArray); // Actually, BM initialization is validate.

	lastoffset_array = (uint32_t*)malloc(sizeof(uint32_t) * nob);
	for (uint32_t i=0; i<nob; i++)
		lastoffset_array[i] = 0;

	block_valid_array = (int8_t*)malloc(sizeof(int8_t) * nob);
	for (i = 0; i < nob; ++i)
		block_valid_array[i] = ERASE; // 0 means ERASED, 1 means VALID

	/* Queue */
#ifdef Queue
	FreeQ = (Queue_t*)malloc(sizeof(Queue_t));
	InitQueue(FreeQ);
	for (int i=0; i<nob; ++i) {
		Enqueue(FreeQ, i);
	}
#endif
	printf("block_create end!\n");

	printf("TOTALSIZE: %ld\n", TOTALSIZE);
	printf("REALSIZE: %ld\n", REALSIZE);
	printf("PAGESIZE: %d\n", PAGESIZE);
	printf("_PPB: %d, ppb = %d, _PPS: %d, BPS: %d\n", _PPB, ppb_, _PPS, BPS);
	printf("BLOCKSIZE: %d\n", BLOCKSIZE);
	printf("_NOP: %ld\n", _NOP);
	printf("_NOS: %ld\n", _NOS);
	printf("_NOB: %ld\n", _NOB);
	printf("_RNOS: %ld\n", _RNOS);
	printf("RANGE: %ld\n", RANGE);
	printf("async: %d\n", ASYNC);
	return 0;

}
void block_destroy (lower_info* li, algorithm *algo){

	free(lastoffset_array);
	free(block_maptable);
	free(block_valid_array);
	//free(sram_valueset);

#ifdef Queue
	free(FreeQ);
#endif

	BM_Free(blockArray);
}
uint32_t block_get(request *const req){
	bench_algo_start(req);

	/* Request production */
	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req=block_end_req;

	uint32_t LBA = my_req->parents->key / ppb_;
	uint32_t offset = my_req->parents->key % ppb_;

	if (block_maptable[LBA] == NIL){
		printf("\nRead Empty Page..\n");
		bench_algo_end(req);
		return 0;
	}

	uint32_t PBA = block_maptable[LBA];
	uint32_t PPA = PBA * ppb_ + offset;

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
	uint32_t LBA = my_req->parents->key / ppb_;
	uint32_t offset = my_req->parents->key % ppb_;
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
#ifdef Queue
		set_pointer = Dequeue(FreeQ);
#endif
#ifdef Linear
		checker = block_findsp(checker);
#endif
		if (set_pointer == (uint32_t)-1) while(1) printf("WHAT!");

		// Switch ERASE to EXIST of block_valid_array
		block_valid_array[set_pointer] = EXIST;

		// Write PBA of mapping result in maptable
		block_maptable[LBA] = set_pointer;

		//PBA = block_maptable[LBA];
		//PPA = PBA * PPB + offset;
		PPA = set_pointer * ppb_ + offset; // Equal to above 2 lines

		if (!BM_IsValidPage(blockArray, PPA))
			while(1)
				printf("ERROROROROROR!\n");
		BM_InvalidatePage(blockArray, PPA);

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
		if (BM_IsValidPage(blockArray, PPA))
		{
#ifdef BFTL_DEBUG1	
			printf("\tcase 2\n"); // 비어있는 page인 경우. 그대로 write 가능하다.
#endif
			int offsetcase = block_CheckLastOffset(lastoffset_array, PBA, offset);
			if (offsetcase == 0){
				// Moving Block with target offset update
				// 새로운 block으로 옮기고 metadata 다 update하기. 사실상 그냥 case 3이랑 똑같음
				while(1) printf("!");
				GC_moving(req, my_req, LBA, offset, PBA, PPA, checker);
			}
			else {
				BM_InvalidatePage(blockArray, PPA); // write되어 차있는 page라고 적어놓는다.
				block_valid_array[PBA] = EXIST;
				bench_algo_end(req);
				__block.li->push_data(PPA, PAGESIZE, req->value, ASYNC, my_req);
			}
		}
		else if (!BM_IsValidPage(blockArray, PPA))
		{
#ifdef BFTL_DEBUG1	
			printf("\tcase 3(GC)\n");
#endif
			GC_moving(req, my_req, LBA, offset, PBA, PPA, checker);
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
	request* res = input->parents;
	res->end_req(res);
	free(input);
	return NULL;
}


void* block_algo_end_req(algo_req* input){
	free(input);
	numLoaded++;
	return NULL;
}

value_set* SRAM_load(uint32_t i, uint32_t old_PPA_zero)
{
	algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	temp_req->parents = NULL;
	temp_req->end_req = block_algo_end_req;
	__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
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
	/* Allocate an empty block */
#ifdef Queue
	set_pointer = Dequeue(FreeQ);
#endif
#ifdef Linear
	checker = block_findsp(checker);
#endif

	/* Maptable updates for data moving */
	block_maptable[LBA] = set_pointer;
	block_valid_array[set_pointer] = EXIST;
	block_valid_array[PBA] = ERASE; // PBA means old_PBA

	uint32_t old_PPA_zero = PBA * ppb_;
	uint32_t new_PBA = block_maptable[LBA];
	uint32_t new_PPA_zero = new_PBA * ppb_;
	uint32_t i = 0;

	/* Start move */
#ifdef BFTL_DEBUG1
	printf("Start move!\n");
#endif

	numLoaded = 0;
	value_set** temp_set = (value_set**)malloc(sizeof(value_set*)*ppb_);
	sram_valueset = (value_set*)malloc(sizeof(value_set) * ppb_);
	memset(sram_valueset, 0, sizeof(value_set) * ppb_);
	//sleep(2);

	if (offset % 4 == 0)
		printf("in GC, offset: %d\n", offset);
	for (i=0; i<ppb_; ++i) { // non-empty page만 옮겨야 하지 않을까? 이것도 상관은 없지만..
#ifdef BFTL_DEBUG3
		if (i % EPOCH == 0)
			printf("SRAM_load: %d\n", i);
#endif
		// SRAM_load
		if (i != offset) {
			temp_set[i] = SRAM_load(i, old_PPA_zero);
#ifdef BFTL_DEBUG2
			printf("temp_set[%d]->value: %c\n", i, *(temp_set[i]->value));
#endif
		}
	}
	//printf("numLoaded: %d\n", numLoaded);

	while (numLoaded != ppb_ - 1) {} // polling for reading all pages in a block

	for (i=0; i<ppb_; ++i) {
#ifdef BFTL_DEBUG3
		if (i % EPOCH == 0)
			printf("memcpy: %d\n", i);
#endif
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


	for (i=0; i<ppb_; ++i) {
#ifdef BFTL_DEBUG3
		if (i % EPOCH == 0)
			printf("SRAM_unload: %d\n", i);
#endif
		// SRAM_unload
		BM_InvalidatePage(blockArray, new_PPA_zero + i);
		BM_ValidatePage(blockArray, old_PPA_zero + i);
		if (i != offset)
			SRAM_unload(i, new_PPA_zero);
		else
			__block.li->push_data(new_PPA_zero + offset, PAGESIZE, req->value, ASYNC, my_req);

	}
	for (i=0; i<ppb_; ++i)
		if (i != offset)
			inf_free_valueset(temp_set[i], FS_MALLOC_R);

	//printf("numLoaded: %d\n", numLoaded);
	/* Trim the block of old PPA */
#ifdef BFTL_DEBUG1
	printf("trim!\n");
#endif
	__block.li->trim_block(old_PPA_zero, false);
#ifdef Queue
	Enqueue(FreeQ, PBA);
#endif
#ifdef BFTL_DEBUG1
	printf("trim end!\n");
#endif
	free(sram_valueset);
}
