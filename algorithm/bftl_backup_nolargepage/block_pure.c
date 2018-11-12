#include "block.h"

/* block ftl with SEGMENT support */
   //need GC algorithm

/* ASYNC ver */
BM_T* BM;
b_queue* bQueue;
Heap* bHeap;
block_table* BT;
segment_table* ST;
segment_table* reservedSeg;
block_OOB* OOB;

uint32_t numLoaded;
int32_t nop_;
int32_t nob_;
int32_t nos_;
int32_t ppb_;
int32_t pps_;
int32_t bps_;

int numGC;

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
	nop_ = _NOP;
	nob_ = _NOB;
	nos_ = _NOS;
	ppb_ = _PPB;
	pps_ = _PPS;
	bps_ = BPS;
	BM = BM_Init(nob_, ppb_, 1, 1); // 1 FreeQueue

	BT = (block_table*)malloc(sizeof(block_table) * nob_);
	ST = (segment_table*)malloc(sizeof(segment_table) * nos_);
	OOB = (block_OOB*)malloc(sizeof(block_OOB) * nop_);

	for (int i=0; i<nob_; i++) {
		BT[i].PBA = NIL;
		BT[i].lastoffset = 0;
		BT[i].valid = 1;
		BT[i].alloc_block = NULL;
	}
	for (int i=0; i<nos_; i++) {
		ST[i].segblock.PBA = i; // PSA(Physical Segment Address)
		ST[i].segblock.Invalid = 0; // number of Invalid blocks in Segment i
		ST[i].segblock.hn_ptr = NULL;
		//ST[i].segblock.type = 0; // meaningless
		//ST[i].segblock.ValidP = NULL; // meaningless
		ST[i].blockmap = (Block**)malloc(sizeof(Block*) * bps_);
		for (int j=0; j<bps_; ++j) {
			ST[i].blockmap[j] = &(BM->barray[i*bps_ + j]);
		}
	}
	for (int i=0; i<nop_; ++i) {
		OOB[i].LPA = NIL;
	}
	

	reservedSeg = &ST[nos_ - 1]; // reserved segment for GC

	BM_Queue_Init(&bQueue);
#if 1
	for(int i = 0; i < nos_ - 1; i++)
		for(int j = 0; j < bps_; j++)
			BM_Enqueue(bQueue, &BM->barray[i * bps_ + j]);
#endif
#if 0
	for (int i=0; i<(nob_-bps_); ++i)
		BM_Enqueue(bQueue, &BM->barray[i]);
#endif
	BM->qarray[0] = bQueue;


	bHeap = BM_Heap_Init(nos_ - 1);
	BM->harray[0] = bHeap;

	printf("block_create end!\n");
	printf("TOTALSIZE: %ld\n", TOTALSIZE);
	printf("REALSIZE: %ld\n", REALSIZE);
	printf("PAGESIZE: %d\n", PAGESIZE);
	printf("_PPB: %d, ppb = %d, _PPS: %d, BPS: %d\n", _PPB, ppb_, _PPS, BPS);
	printf("BLOCKSIZE: %d\n", BLOCKSIZE);
	printf("_NOP: %ld\n", _NOP);
	printf("_NOS: %ld\n", _NOS);
	printf("_nob_: %ld\n", _NOB);
	printf("_RNOS: %ld\n", _RNOS);
	printf("RANGE: %ld\n", RANGE);
	printf("async: %d\n", ASYNC);
	return 0;

}
void block_destroy (lower_info* li, algorithm *algo){
	free(BT);
	for (int i=0; i<nos_; ++i)
		free(ST[i].blockmap);
	free(ST);
	free(OOB);

	printf("block_destroy() called.. Total number of GC: %d\n", numGC);

	BM_Free(BM);
}
uint32_t block_get(request *const req){
	uint32_t LBA;
	uint32_t offset;
	uint32_t PBA;
	uint32_t PPA;
	//algo_req* my_req;

	bench_algo_start(req);

	/* Request production */
	//my_req=(algo_req*)malloc(sizeof(algo_req));
	//my_req->parents = req;
	//my_req->end_req=block_end_req;

	LBA = req->key / ppb_;
	offset = req->key % ppb_;
	PBA = BT[LBA].PBA;
	PPA = PBA * ppb_ + offset;

	if (!BM_IsValidPage(BM, PPA)) {
		bench_algo_end(req);
		printf("\nRead Empty Page..\n");
		req->type = FS_NOTFOUND_T;
		req->end_req(req);
		return 1;
	}

	bench_algo_end(req);
	//__block.li->pull_data(PPA, PAGESIZE, req->value, ASYNC, my_req);
	__block.li->pull_data(PPA, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_R, NULL, req));

	return 0;
}

uint32_t block_set(request *const req){
	uint32_t LBA;
	uint32_t offset;
	uint32_t PBA;
	uint32_t PPA;
	uint32_t PSA;
	//algo_req* my_req;
	segment_table* seg_;

	bench_algo_start(req);

	/* Request production */
	//my_req = (algo_req*)malloc(sizeof(algo_req));
	//my_req->parents = req;
	//my_req->end_req = block_end_req;

	LBA = req->key / ppb_;
	offset =req->key % ppb_;

	if (BT[LBA].PBA == NIL)
	{
		BT[LBA].alloc_block = BM_Dequeue(bQueue);
		if (BT[LBA].alloc_block == NULL) {
			block_GC();
			BT[LBA].alloc_block = BM_Dequeue(bQueue);
		}
		BT[LBA].PBA = BT[LBA].alloc_block->PBA;
		seg_ = &ST[BT[LBA].PBA/bps_];
		if (!seg_->segblock.hn_ptr)
			seg_->segblock.hn_ptr = BM_Heap_Insert(bHeap, &seg_->segblock);
#if 0
		PSA = LBA_TO_PSA(BT, LBA);
		if (ST[PSA].segblock.hn_ptr == NULL) {
			ST[PSA].segblock.hn_ptr = BM_Heap_Insert(bHeap, &ST[PSA].segblock);
		}
#endif
	}
	PBA = BT[LBA].PBA;
	PPA = PBA * ppb_ + offset;
	if (!block_CheckLastOffset(LBA, offset) || BM_IsValidPage(BM, PPA)) {
		// Bad ASC order or Page Collision occurs
		//GC_moving(req, my_req, LBA, offset, PBA, PPA);
		GC_moving(req, NULL, LBA, offset, PBA, PPA);
		bench_algo_end(req);
		req->end_req(req);
	}
	else {
		// case: We can write the page directly
		BM_ValidatePage(BM, PPA);
		OOB[PPA].LPA = req->key; // save LPA
		BT[LBA].lastoffset = offset;
		bench_algo_end(req);
		__block.li->push_data(PPA, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_W, NULL, req));
	}

	return 0;
}
uint32_t block_remove(request *const req){
	return 0;
}

#if 0
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
void* block_algo_end_req2(algo_req* input){
	free(input);
	return NULL;
}
#endif

void* block_end_req(algo_req* input){
	block_params *params = (block_params*)(input->params);
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
