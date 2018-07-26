#include "block.h"
#include "../../bench/bench.h"

/* ASYNC ver */

int32_t nob_;
int32_t ppb_;
int32_t numLoaded;
BM_T* BM;

struct algorithm __block={
	.create=block_create,
	.destroy=block_destroy,
	.get=block_get,
	.set=block_set,
	.remove=block_remove
};

uint32_t block_create (lower_info* li,algorithm *algo){
	printf("block_create start!\n");
	algo->li=li;
	//BM_Init(&blockArray);
	nob_ = _NOS;
	ppb_ = _PPS;

	printf("async: %d\n", ASYNC);
	printf("# of page: %ld\n", _NOP);
	printf("# of block: %ld\n", nob_);
	printf("page per block: %ld\n", ppb_);

	BM = BM_Init(0, 0);
	block_maptable = (int32_t*)malloc(sizeof(int32_t) * nob);
	uint32_t i=0;
	for (; i<nob; ++i){ // maptable initialization
		block_maptable[i] = NIL;
	}

	//BM_ValidateAll(BM->barray); // Actually, BM initialization is validate.

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

	BM_Free(BM);
}

void* block_end_req(algo_req* input){
	block_params *params = (block_params*)input->params;
	value_set *temp_set = params->value;
	request *res = input->parents;

	switch(params->type){
		case DATA_R:
			if(res){
				res->end_req(res);
			}
			break;
		case DATA_W:
			if(res){
				res->end_req(res);
			}
			break;
		case GC_R:
			numLoaded++;
			break;
		case GC_W:
			inf_free_valueset(temp_set, FS_MALLOC_W);
			break;
	}
	free(params);
	free(input);
	return NULL;
}

uint32_t block_get(request *const req){
	int32_t lba;
	int32_t lpa;
	int32_t pba;
	int32_t ppa;
	int32_t offset;

	bench_algo_start(req);
	lpa = req->key;
	lba = lpa / ppb_;
	offset = lpa % ppb_;

	if(!BM_IsValidPage(BM, lpa)){
		bench_algo_end(req);
		req->type = FS_NOTFOUND_T;
		req->end_req(req);
		return 1;
	}
	pba = block_maptable[lba];
	ppa = pba * ppb_ + offset;
	bench_algo_end(req);
	__block.li->pull_data(ppa, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_R, NULL, req));
	return 0;
}

uint32_t block_set(request *const req){
	algo_req* my_req;
	bench_algo_start(req);

	/* Request production */
	my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = block_end_req;
#ifdef BFTL_KEYDEBUG
	if (my_req->parents->key % 1000 == 0)
		printf("Start set! key: %d\n", my_req->parents->key);
#endif
	//printf("seq:%d\n",set_seq++);
	//printf("block_set 1!\n");
	uint32_t LBA = my_req->parents->key / ppb_;
	uint32_t offset = my_req->parents->key % ppb_;
	uint32_t PBA;
	uint32_t PPA;
	int8_t checker = 0;

	//printf("Start set! key: %d, LBA: %d, offset: %d\n", my_req->parents->key, LBA, offset);

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

		if (BM_IsValidPage(BM, PPA))
			while(1)
				printf("ERROROROROROR!\n");
		BM_ValidatePage(BM, PPA);

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
		PPA = PBA  * ppb_ + offset;

		// data가 차있는 page를 valid라고 할 지 invalid라고 할 지 제대로 골라야 할 것 같다. 지금은 차있는 page를 invalid라고 설정하여 그 page에 write하려고 할 시에 GC를 하도록 하였다. valid로 바꾸는 게 맞는 것 같긴 한데, 그렇게 하면 write할 수 있는(비어있는) page들은 모두 invalid 상태라고 여기는 것이 된다. 그건 또 이상하다.. 그냥 block ftl에서는 write할 수 있는(비어있는) page를 valid page로, write할 수 없는(쓰여있는) page를 invalid page로 여기고 BM을 사용한다고 여기면 될 것 같다.
		// 그리고 block level에서의 valid/invalid 여부는 BM valid/invalid와 상관없다. 각 page마다 0/1을 구분하기 위해 BM의 page 단위 validity를 쓴 것일 뿐이다.
		if (!BM_IsValidPage(BM, PPA))
		{
#ifdef BFTL_DEBUG1	
			printf("\tcase 2\n"); // 비어있는 page인 경우. 그대로 write 가능하다.
#endif
			int offsetcase = block_CheckLastOffset(lastoffset_array, PBA, offset);
			if (offsetcase == 0){
				// Moving Block with target offset update
				// 새로운 block으로 옮기고 metadata 다 update하기. 사실상 그냥 case 3이랑 똑같음
				GC_moving(req, my_req, LBA, offset, PBA, PPA, checker);
			}
			else {
				BM_ValidatePage(BM, PPA); // write되어 차있는 page라고 적어놓는다.
				block_valid_array[PBA] = EXIST;
				bench_algo_end(req);
				__block.li->push_data(PPA, PAGESIZE, req->value, ASYNC, my_req);
			}
		}
		else if (BM_IsValidPage(BM, PPA))
		{
#ifdef BFTL_DEBUG1	
			printf("\tcase 3(GC)\n");
#endif
			GC_moving(req, my_req, LBA, offset, PBA, PPA, checker);
		}
		else
			while(1) printf("Valid???");
	}
#ifdef BFTL_DEBUG1
	printf("end)LBA: %d, offset: %d\n", LBA, offset);
#endif
#if 0
	if (offset > 16375){
		printf("LBA: %d, offset: %d\n", LBA, offset);
		printf("PBA: %d, PPA: %d\n", PBA, PPA);
		sleep(1);
	}
#endif
	
	return 0;

}

uint32_t block_remove(request *const req){
	//	block->li->trim_block()
	return 0;
}
