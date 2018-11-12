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

// for write buffer
sem_t empty;
sem_t full;
sem_t mutex;
request ** wb;
request** req_set;
int fill;
int wb_empty;
int wb_hit;


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
	nop_ = _NOP * RPP;
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
	printf("nop_: %d\n", nop_);
	printf("_NOS: %ld\n", _NOS);
	printf("_nob_: %ld\n", _NOB);
	printf("_RNOS: %ld\n", _RNOS);
	printf("RANGE: %ld\n", RANGE);
	printf("async: %d\n", ASYNC);


	wb = (request**)malloc(sizeof(request*)*WB_SIZE);
	for (int i=0; i<WB_SIZE; ++i)
		wb[i] = NULL;
	req_set = (request**)malloc(sizeof(request*)*RPP);
	for (int i=0; i<RPP; ++i)
		req_set[i] = NULL;

	sem_init(&mutex, 0, 1);

	return 0;
}
void block_destroy (lower_info* li, algorithm *algo){
	free(BT);
	for (int i=0; i<nos_; ++i)
		free(ST[i].blockmap);
	free(ST);
	free(OOB);

	printf("block_destroy() called.. Total number of GC: %d\n", numGC);
	printf("wb_hit: %d\n", wb_hit);

	BM_Free(BM);
	free(wb);
	free(req_set);
}
uint32_t block_get(request *const req){
	uint32_t LBA;
	uint32_t offset;
	uint32_t PBA;
	uint32_t PPA;
	uint8_t offset_req;
	//algo_req* my_req;

	bench_algo_start(req);

	// First, check the write buffer and if the req exists, hand over it
	if (!wb_empty) {
		for (int i=WB_SIZE-1; i>=0; --i) {
			//printf("i: %d\n", i);
			//printf("wb[%d]->key: %d\n", i, wb[i]->key);
			//printf("req->key: %d\n", req->key);
			if (wb[i]->key == req->key) {
				bench_algo_end(req);
				printf("wb_get hit!\n");
				wb_hit++;
				memcpy(req->value->value, wb[i]->value->value, PAGESIZE);
				return 2;
			}
		}
	}
	// mapping table을 보고 lpa에 해당하는 ppa를 찾을 때,
	// 그 ppa에는 2개(RPP개)의 request value가 들어 있으니까 
	// 결국 mapping table에 그 중 어느 건지 적어놓아야 한다.
	// 결국.. lpa마다 그게 왼쪽인지 오른쪽인지 적어놓아야 하는 거 아닌가. (lpa) bits만큼 필요.


	/* Request production */
	//my_req=(algo_req*)malloc(sizeof(algo_req));
	//my_req->parents = req;
	//my_req->end_req=block_end_req;

	LBA = req->key / ppb_;
	offset = req->key % ppb_;
	PBA = BT[LBA].PBA;
	PPA = PBA * ppb_ + offset;
	offset_req = BT[LBA].req_offset[offset];

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
	// pull에서 어떻게 하지? physical page의 해당 ppa에 대하여, offset_req번째 구획에서의 데이터만 req->value에 넣어야 하는데..

	return 0;
}

uint32_t block_set(request *const req){
	sem_wait(&mutex);
	wb[fill++] = req;
	wb_empty = 0;
	sem_post(&mutex);
	printf("req: 0x%x\n", req);
	//printf("block_set start!\n");

	if (fill == WB_SIZE) {
		wb_full();
		fill = 0;
	}
}
	

void __block_set_double(request** req_set){

void __block_set_internal(request *const req){
	uint32_t LBA;
	uint32_t offset;
	uint32_t PBA;
	uint32_t PPA;
	uint32_t PSA;
	segment_table* seg_;

	bench_algo_start(req);

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

	//return 0;
}
void __block_set(request **req_set){
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

	//return 0;
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

void wb_full(void)
{
	// If write buffer is full, 
	// 1. if possible, merge some entries in buffer
	// 2. flush buffer entries.. with compaction for large page support

	// for now, let's implement buffer flush without compaction
	//printf("wb_full start!\n");
	for (int i=0; i<WB_SIZE; ++i) {
		__block_set(wb[i]);
		wb[i] = NULL;
		wb_empty = 1;
	}

	// 여기서 두개씩 합쳐서 set으로 넣어야겠네.
	for (int i=0; i<WB_SIZE/RPP; ++i) {
		for (int j=0; j<RPP; ++j) {
			req_set[j] = wb[i*RPP + j];
		}
		__block_set(req_set);
	}
}






/*
   LPA 범위를 두배로 늘려서
   NOP * 2로 받아오면.. nop는 실제 NOP보다 2배로 되어있고
   pull의 인자로는 PPA/2를 넣고..
   req의 params에 PPA%2를 넣어두면 되겠네.
   어차피 device buffer에는 한번에 8K씩 가져오니까 합쳐져 있을 수밖에 없고
   지금 FlashSimulator는 user로 읽어오는 게 구현되어있지는 않으니까
   일단은 표시만 해두면 될 것 같다.

	oop는 일단 fake ppa(2배 상태)만큼 생기니까 신경 쓸 필요 없을 것 같다.
   */


