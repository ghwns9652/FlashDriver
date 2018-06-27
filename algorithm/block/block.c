#include "block.h"

/* ASYNC ver */

struct algorithm __block={
	.create=block_create,
	.destroy=block_destroy,
	.get=block_get,
	.set=block_set,
	.remove=block_remove
};

/* Set set_pointer to first-meet ERASE index from current set_pointer like a round-robin */
int32_t block_findsp(int32_t checker){
	// temporary..
	//if (set_pointer > 0)
		//set_pointer--;
	set_pointer = 0;

	// Linked list나 Queue 관련해서 생각해 보기

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

	//printf("block_create End!\n");
	pthread_mutex_init(&lock, NULL);
	return 0;

}
void block_destroy (lower_info* li, algorithm *algo){

	free(block_maptable);
	free(block_valid_array);

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

	__block.li->pull_data(PPA, PAGESIZE, req->value, 0, my_req);
	bench_algo_end(req);

	return 0;
}
static int set_seq=0;
uint32_t block_set(request *const req){
	bench_algo_start(req);

	//printf("block_set Start!\n");
	/* Request production */
	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = block_end_req;

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

	if (block_maptable[LBA] == NIL)
	{
		checker = block_findsp(checker);

		// Switch E to V of block_valid_array
		block_valid_array[set_pointer] = VALID;

		// Write PBA of mapping result in maptable
		block_maptable[LBA] = set_pointer;

		//PBA = block_maptable[LBA];
		//PPA = PBA * PPB + offset;
		PPA = set_pointer * __block.li->PPB + offset; // Equal to above 2 lines

		if (!BM_is_valid_ppa(blockArray, PPA))
			while(1)
				printf("ERROROROROROR!\n");
		BM_invalidate_ppa(blockArray, PPA);

		// write
		//printf("block_set 2!\n");
		//printf("my_req -> length: %d\n", my_req->parents->value->length);
		__block.li->push_data(PPA, PAGESIZE, req->value, 0, my_req);
		//printf("block_set 3!\n");
	}

	else
	{
		PBA = block_maptable[LBA];
		PPA = PBA  * __block.li->PPB + offset;


		if (BM_is_valid_ppa(blockArray, PPA))
		{
			BM_invalidate_ppa(blockArray, PPA);
			block_valid_array[PBA] = VALID;
			__block.li->push_data(PPA, PAGESIZE, req->value, 0, my_req);
		}
		else if (!BM_is_valid_ppa(blockArray, PPA))
		{
			// Cleaning
			// Maptable update for data moving
#if 0
			printf("Start GC!\n");
			printf("offset: %d\n", offset);
			printf("PBA: %d\n", PBA);
			printf("PPA: %d\n", PPA);
			//exit(1);
#endif

			checker = block_findsp(checker);
			block_maptable[LBA] = set_pointer;
			block_valid_array[set_pointer] = VALID;
			block_valid_array[PBA] = ERASE; // PBA means old_PBA

			uint32_t old_PPA_zero = PBA * __block.li->PPB;
			uint32_t new_PBA = block_maptable[LBA];
			uint32_t new_PPA_zero = new_PBA * __block.li->PPB;
			uint32_t new_PPA = new_PBA * __block.li->PPB + offset;

			// Data move to new block
			uint32_t i;

			/* Followings: ASC consideartion */

			/* Start move(smaller index than target) */
			//printf("Start move 1!\n");
			// GC 부분을 함수로 빼기
			for (i = 0; i < offset; ++i) {
				printf("i: %d\n");
				/* Allocate temporary request, value set for read */
				algo_req *temp_req=(algo_req*)malloc(sizeof(algo_req));
				//printf("before inf_get_valueset, PAGESIZE: %d\n", PAGESIZE);
				value_set* temp_read_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
				//printf("after inf_get_valueset, PAGESIZE: %d\n", PAGESIZE);
				temp_req->parents = NULL;
				temp_req->end_req = block_algo_end_req;

			pthread_mutex_lock(&lock);
				__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_read_value_set, ASYNC, temp_req);
				//printf("after gc pull\n");
			pthread_mutex_lock(&lock);
			pthread_mutex_unlock(&lock);


				/* Validate old PPA and Invalidate new PPA */
				BM_validate_ppa(blockArray, old_PPA_zero + i);
				BM_invalidate_ppa(blockArray, new_PPA_zero + i);


				/* Allocate temporary request, value set for write */
				algo_req *temp_req2=(algo_req*)malloc(sizeof(algo_req));
				value_set* temp_write_value_set = inf_get_valueset(temp_read_value_set->value, FS_MALLOC_W, PAGESIZE);
				temp_req2->parents = NULL;
				temp_req2->end_req = block_algo_end_req;

			pthread_mutex_lock(&lock);
				__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_write_value_set, ASYNC, temp_req2);
			pthread_mutex_lock(&lock);
			pthread_mutex_unlock(&lock);

				/* Free valueset */
				inf_free_valueset(temp_write_value_set, FS_MALLOC_W);
				inf_free_valueset(temp_read_value_set, FS_MALLOC_R);
			}
			//printf("End move 1!\n");

			//printf("Start push!\n");
			pthread_mutex_lock(&lock);
			__block.li->push_data(new_PPA, PAGESIZE, req->value, 0, my_req);
			pthread_mutex_lock(&lock);
			pthread_mutex_unlock(&lock);
			//printf("End push!\n");
			//printf("Start validate, invalidate in BM!\n");
			BM_validate_ppa(blockArray, old_PPA_zero + offset);
			BM_invalidate_ppa(blockArray, new_PPA_zero + offset);
			//printf("End validate, invalidate in BM!\n");

			
			/* Start move(bigger index than target) */
			//printf("Start move 2!\n");
			if (offset < __block.li->PPB - 1) {
				for (i = offset + 1; i < __block.li->PPB; ++i) {
				//printf("i:%d\n",i);
					/* Allocate temporary request, value set for read */
					algo_req *temp_req = (algo_req*)malloc(sizeof(algo_req));
					value_set* temp_read_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
					temp_req->parents = NULL;
					temp_req->end_req = block_algo_end_req;

			pthread_mutex_lock(&lock);
					__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_read_value_set, 0, temp_req);
			pthread_mutex_lock(&lock);
			pthread_mutex_unlock(&lock);


					/* Validate old PPA and Invalidate new PPA */
					BM_validate_ppa(blockArray, old_PPA_zero + i);
					BM_invalidate_ppa(blockArray, new_PPA_zero + i);


					/* Allocate temporary request, value set for write */
					algo_req *temp_req2=(algo_req*)malloc(sizeof(algo_req));
					value_set* temp_write_value_set = inf_get_valueset(temp_read_value_set->value, FS_MALLOC_W, PAGESIZE);
					temp_req2->parents = NULL;
					temp_req2->end_req = block_algo_end_req;

			pthread_mutex_lock(&lock);
					__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_write_value_set, 0, temp_req2);
			pthread_mutex_lock(&lock);
			pthread_mutex_unlock(&lock);

					/* Free valueset */
					inf_free_valueset(temp_write_value_set, FS_MALLOC_W);
					inf_free_valueset(temp_read_value_set, FS_MALLOC_R);
				}
			}
			//printf("End move 2!\n");
					
			/* Trim the block of old PPA */
			//printf("trim!\n");
			__block.li->trim_block(old_PPA_zero, false); // trim is corrupted: wait master update
			//printf("trim end!\n");
			pthread_mutex_unlock(&lock);
		}
	}
	bench_algo_end(req);

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
	pthread_mutex_unlock(&lock);
	return NULL;
}



/* From gyeongtaek idea */
void* block_algo_end_req(algo_req* input){
	free(input);
	return NULL;
}
