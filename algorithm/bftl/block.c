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

int numREP;
int numNIL;
int numIVP;

uint32_t numLoaded;
int32_t nop_;
int32_t nob_;
int32_t nos_;
int32_t ppb_;
int32_t pps_;
int32_t bps_;
int32_t bps__;

int numGC;
int numCompact;

struct algorithm __block={
	.create=block_create,
	.destroy=block_destroy,
	.get=block_get,
	.set=block_set,
	.remove=block_remove
};


#define SMALLB
uint32_t block_create (lower_info* li,algorithm *algo){
	printf("block_create start!\n");
	algo->li=li;
	nop_ = _NOP;
	nob_ = _NOB;
	nos_ = _NOS;
	ppb_ = _PPB;
	pps_ = _PPS;
	bps_ = BPS;

#ifdef NOB_MODE
	nob_ *= RPP;
	bps_ *= RPP;
	//bps__ = bps_ * RPP;
	//BM = BM_Init(nob_, ppb_/RPP, 1, 1); // 1 FreeQueue
	BM = BM_Init(nob_, ppb_, 1, 1); // 1 FreeQueue
	//BM = BM_Init(nob_/RPP, ppb_, 1, 1); // 1 FreeQueue
#endif
#ifdef PPB_MODE
	ppb_ *= RPP;
	bps_ /= RPP;
	nob_ /= RPP;
	
	BM = BM_Init(nob_, ppb_, 1, 1);
#endif
#ifdef SMALLB
	ppb_ /= 2;
	nob_ *= 2;
	bps_ *= 2;
	BM = BM_Init(nob_, ppb_, 1, 1);
#endif

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
		OOB[i].LPA2 = NIL;
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

	wb_empty = 1;
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
	printf("numCompact: %d\n", numCompact);

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

	uint32_t req_lpa = req->key;

	bench_algo_start(req);

	// First, check the write buffer and if the req exists, hand over it
	if (!wb_empty) {
		//for (int i=WB_SIZE-1; i>=0; --i) {
		for (int i=fill-1; i>=0; --i) {
			//printf("i: %d\n", i);
			//printf("wb[%d]->key: %d\n", i, wb[i]->key);
			//printf("req->key: %d\n", req->key);
			if (wb[i]->key == req_lpa) {
				bench_algo_end(req);
				printf("wb_get hit!\n");
				wb_hit++;
				memcpy(req->value->value, wb[i]->value->value, PAGESIZE);
				req->end_req(req);
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

	LBA = req->key / (ppb_*2);
	offset = req->key % (ppb_*2); // now, LPA offset
	if (BT[LBA].PBA >= nob_) {
		printf("error!! BlockTable is wrong. LBA: %d, PBA: %d\n", LBA, BT[LBA].PBA);
		exit(0);
	}
	PBA = BT[LBA].PBA;
	PPA = PBA * ppb_ + offset/2;
	//offset_req = BT[LBA].req_offset[offset];

	printf("Before IsValidPage, LPA: %d, LBA: %d, PBA: %d, PPA: %d\n", req->key, LBA, PBA, PPA);
	if (((int32_t)PBA == NIL) || !BM_IsValidPage(BM, PPA)) {
		// Randrw에서 이게 발생하면 안 된다. LBA->PBA의 mapping table이 제대로 만들어지지 못했다.
		bench_algo_end(req);
		printf("\nRead Empty Page..\n");
		numREP++;
		if ((int32_t)PBA==NIL) numNIL++;
		else if (!BM_IsValidPage(BM,PPA)) numIVP++;
		req->type = FS_NOTFOUND_T;
		req->end_req(req);
		while(1) {;}
		exit(5);
		return 1;
	}

	bench_algo_end(req);
	//__block.li->pull_data(PPA, PAGESIZE, req->value, ASYNC, my_req);
	__block.li->pull_data(PPA, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_R, NULL, req));
	// pull_data를 하면 req->value->value가 x가 아니다.. 애초에 처음부터 그러는 걸 보니 기본적인 set 자체가 이루어지고 있지 않는 듯하다.

	// pull에서 어떻게 하지? physical page의 해당 ppa에 대하여, offset_req번째 구획에서의 데이터만 req->value에 넣어야 하는데..
	//printf("req->value->value: %x\n", req->value->value);
	//printf("req->value->value: %c\n", *(req->value->value));
	char* first;
	char* second;
	//memcpy(first, req->value->value, PAGESIZE/2);
	//memcpy(second, req->value->value + PAGESIZE/2, PAGESIZE/2);
	first = req->value->value;
	//second = first + PAGESIZE/2;
	second = first + (1<<12);
	//printf("fisrt: %c, second: %c\n", *first, *second);
	//printf("fisrt: %c\n", *first);
	//printf("second: %c\n", *second);

	return 0;
}

uint32_t block_set(request *const req){
	sem_wait(&mutex);
	wb[fill++] = req;
	wb_empty = 0;
	sem_post(&mutex);
	//printf("req: 0x%x\n", req);
	//printf("block_set start!\n");

	if (fill == WB_SIZE) {
		wb_full();
		fill = 0;
	}
	// wb 덜 남았을 때도 처리할 수 있게.. 그리고 내려오는 request 자체가 WB_SIZE보다 적을 때도 고려해야..
	return 0;
}
	

//void __block_set_double(request** req_set){

#if 0
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
#endif
void __block_set(request **req_set){
//void __block_set(request *req){
	uint32_t LBA;
	uint32_t offset, offset2;
	uint32_t PBA;
	uint32_t PPA, PPA2;
	uint32_t PSA;
	int8_t compact = 0;
	//algo_req* my_req;
	segment_table* seg_;

	if (req_set[1])
		compact = 1;

	//bench_algo_start(req);
	bench_algo_start(req_set[0]);
	if (compact)
		bench_algo_start(req_set[1]);

	/* Request production */
	//my_req = (algo_req*)malloc(sizeof(algo_req));
	//my_req->parents = req;
	//my_req->end_req = block_end_req;

	LBA = req_set[0]->key / (ppb_*2);
	offset = req_set[0]->key % (ppb_*2);
	if (compact) {
		offset2 = req_set[1]->key % (ppb_*2); // == offset+1
		if (offset2 != offset+1) {
			printf("offset2 is wrong! offset: %d, offset2: %d\n", offset, offset2);
			exit(1);
		}
	}


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
	offset /= 2; // offset should be ppa offset
	if (compact) offset2 /= 2;
	// offset도 LPA용과 PPA용을 구분해서 변수를 두는 게 낫을듯. ppb도.

	PBA = BT[LBA].PBA;
	PPA = PBA * ppb_ + offset;
	if (compact) PPA2= PBA * ppb_ + offset2; // == PPA+1; // PPA와 PPA2는 동일해야 하므로 사실 이건 쓰지 못할 거 같다.
	// bftl과 BM이 보는 ppb가 다른데, PPA가 제대로 될 까? 
	// BM에서 PPA를 어떻게 처리하는 지 보자.
#if 1
	printf("BT[%d].PBA = %d\n", LBA, PBA);
	printf("PPA: %d\n", PPA);
#endif
	if (!block_CheckLastOffset(LBA, offset) || BM_IsValidPage(BM, PPA)) {
		// Bad ASC order or Page Collision occurs
		//GC_moving(req, my_req, LBA, offset, PBA, PPA);
		GC_moving(req_set, NULL, LBA, offset, PBA, PPA);
		bench_algo_end(req_set[0]);
		if (compact) bench_algo_end(req_set[1]);
		req_set[0]->end_req(req_set[0]);
		//if (compact) req_set[1]->end_req(req_set[1]);
	}
	else {
		// case: We can write the page directly
		BM_ValidatePage(BM, PPA);
		OOB[PPA].LPA = req_set[0]->key; // save LPA
		BT[LBA].lastoffset = offset;
		bench_algo_end(req_set[0]);
		if (compact) {
			//BM_ValidatePage(BM, PPA2); // == PPA+1
			//OOB[PPA2].LPA = req_set[1]->key; // save LPA
			OOB[PPA].LPA2 = req_set[1]->key;
			bench_algo_end(req_set[1]); // 이거 12월 전에는 주석처리였다
			//memcpy(req_set[0]->value->value + (PAGESIZE/2), req_set[1]->value->value, PAGESIZE/2); // add value of req1 to value of req0
			memcpy((req_set[0]->value->value) + (1<<12), req_set[1]->value->value, (1<<12)); // add value of req1 to value of req0
			printf("breakpoint 확인용\n");
			printf("req_set[0]->value->value: %x(%c)\n", req_set[0]->value->value, *(req_set[0]->value->value));
			printf("req_set[0]->value->value + (1<<12): %x(%c)\n", req_set[0]->value->value + (1<<12), *(req_set[0]->value->value + (1<<12)));
			printf("req_set[0]->value->value + (1<<13)-1: %x(%c)\n", req_set[0]->value->value + (1<<13)-1, *(req_set[0]->value->value + (1<<13)-1));
			printf("req_set[0]->value->value + (1<<13): %x(%c)\n", req_set[0]->value->value + (1<<13), *(req_set[0]->value->value+ (1<<13)));
			// (12.7) 이 memcpy 이후 req_set[0]->value->value가 'x'가 아니게 된 것을 확인하였음
			// req_set[0] + (1<<13)보다 req_set[1]이 조금 더 커서, req_set[0] + (1<<12)에서 1<<12만큼 memcpy하다보면 금방 req_set[1]이랑 겹친다. memcpy는 겹치면 안 되기에 문제가 생기는 것 같다. 아니 안겹치지 않나?
		}
		__block.li->push_data(PPA, PAGESIZE, req_set[0]->value, ASYNC, assign_pseudo_req(DATA_W, NULL, req_set[0])); // 처리한 request 개수는 2개로 세어줘야 할 텐데.. 어떻게?
		sleep(5);
		// 이 push_data에서 PAGESIZE만큼 하니까 바롱 위세ㅓ 한 req_set[1] memcpy가 덮여씌워지지 않을까?
		// push 직전에 req_set[0] 안에다가 아예 req_set[1] 내용을 덮어씌우고 나서 push에서 req_set[0]을 그냥 쓰는거니까 이건 문제가 아닐 것 같다.
		// push 이후에 req_set[0]->value->value가 이상하게 바뀌는 것 같기도 하다.
		// 위의 memcpy 문제가 아니라 push_data 문제인 것 같다. push_data 직후에 req_set[0]->value->value가 이상하게 바뀌어버린다. 아 근데 생각해보면 애초에 device에 적고 나서 request는 end_req 해버릴테니까 사라지는 게 당연한거긴 하다. 흠..
		// end_req가 있긴 할텐데 사라지는 게 맞나? pull이라면 req로 가져오는건데 사라지면 안되는 거 아닌가? 애초에 pull은 어디로 가져오는 거지? req->value로 PPA에 해당하는 내용물을 read해서 받아오는 게 맞다. end_req와는 별개일 것. 
		// gdb로 보니, push 이후 req_set[0]->value는 그대로지만 req_set[0]->value->value는 확 변한다. req_set[0]->value->value를 read()해서 device 안에 넣는 건 맞지만..
		// push는 변해도 큰 문제는 없겠지만, pull에서도 변한다면 문제가 있다고 할 수 있다. 애초에 bench에서 pull로 꺼낸 걸 확인하는 게 있나? 
	}
	// test용 pull
	value_set** temp_set;
	block_sram* sram_table;
	temp_set = (value_set**)malloc(sizeof(value_set*) * ppb_);
	sram_table = (block_sram*)malloc(sizeof(block_sram) * ppb_);
	
	for (int i=0; i<ppb_; ++i) {
		sram_table[i].SRAM_OOB.LPA = NIL;
		sram_table[i].SRAM_OOB.LPA2 = NIL;
		sram_table[i].SRAM_PTR= NULL;
		temp_set[i] = NULL;
	}
	temp_set[0] = SRAM_load(sram_table, PPA, 0);
	printf("temp_set[0]->value: %c\n", temp_set[0]->value);
	memcpy(sram_table[0].SRAM_PTR, temp_set[0]->value, PAGESIZE);
	inf_free_valueset(temp_set[0], FS_MALLOC_R);
	printf("sram_table[0]->SRAM_PTR: %c\n", sram_table[0].SRAM_PTR);
	sleep(5);
	printf("after)sram_table[0]->SRAM_PTR: %c\n", sram_table[0].SRAM_PTR);
	// SRAM_load의 결과도 이상한 거 보니, 애초에 push 자체가 제대로 안 됐다.
	// push 사용법이 맞는지부터 다시 봐야겠다.
	
	value_set* test_set;
	test_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	__block.li->pull_data(PPA, PAGESIZE, test_set, 1, assign_pseudo_req(GC_R, NULL, NULL)); // pull in gc is ALWAYS async
	printf("after)test_set->SRAM_PTR: %c\n", test_set->value);

	if (compact) {
		req_set[1]->end_req(req_set[1]); //// 여기서 rand test 터진다.. 11.06
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
#ifdef WB_DEBUG
	printf("wb_full start!\n");
#endif
#if 0
	for (int i=0; i<WB_SIZE; ++i) {
		__block_set(wb[i]);
		wb[i] = NULL;
		wb_empty = 1;
	}
#endif
	// 1. sort
	__merge_sort(wb, WB_SIZE);
#if 0
	for (int i=0; i<100; i++)
		printf("wb(1~10): %x, key: %d\n", wb[i], wb[i]->key);

	__merge_sort(wb, WB_SIZE);
	printf("After merge,\n");

	for (int i=0; i<100; i++)
		printf("wb(1~10): %x, key: %d\n", wb[i], wb[i]->key);

	exit(0);
#endif
	int count = 0;

	// 2. clustering with compaction
	for (int i=0; i<WB_SIZE- 1; ++i) {
		// agressively, set req_set[0]
		req_set[0] = wb[i];

		// lba check
		if (wb[i]->key/ppb_ == wb[i+1]->key/ppb_) {
#ifdef WB_DEBUG
			printf("lba check pass\t");
#endif
			// offset check
			if ((wb[i]->key % ppb_)/2 == (wb[i+1]->key % ppb_)/2) {
#ifdef WB_DEBUG
				printf("offset check pass\t");
#endif
				// same lpa check
				if (wb[i]->key == wb[i+1]->key) {
#ifdef WB_DEBUG
					printf("ignore old request for %x(%d), new request %x(%d)\n", wb[i], wb[i]->key, wb[i+1], wb[i+1]->key);
#endif
					continue;
				} else {
					// this code is so dirty.. but work
#ifdef WB_DEBUG
					printf("samelpa check pass. all pass\n");
#endif
					for (int k=1; i+1+k <WB_SIZE; k++) {
						if (wb[i+1]->key != wb[i+1+k]->key)
							break;
						else 
							count++;
					}
					req_set[1] = wb[i+1+count];
					i += 1+count;
					assign_pseudo_req(DATA_W, NULL, req_set[1]);

					numCompact++;
					//req_set[1]->end_req = block_end_req;
					//req_set[1]->end_req(req_set[1]);

					// 여기에 request 하나 끝났다고 해줘야..
					//__block.li->push_data(0, PAGESIZE, req_set[1]->value, ASYNC, assign_pseudo_req(DATA_W, NULL, req_set[1])); // 처리한 request 개수는 2개로 세어줘야 할 텐데.. 어떻게?
		//__block.li->push_data(PPA, PAGESIZE, req_set[0]->value, ASYNC, assign_pseudo_req(DATA_W, NULL, req_set[0])); // 처리한 request 개수는 2개로 세어줘야 할 텐데.. 어떻게?
				}
			}
			else {
#ifdef WB_DEBUG
				printf("offset check fail. no compaction\n");
#endif
				for (int j=1; j<RPP; ++j)
					req_set[j] = NULL;
			}

		} else { // no compaction
			//req_set[0] = wb[i];
#ifdef WB_DEBUG
			printf("lba check fail. no compaction\n");
#endif
			for (int j=1; j<RPP; ++j)
				req_set[j] = NULL;
		}
#ifdef WB_DEBUG
		printf("req0: %x(%d)\t", req_set[0], req_set[0]->key);
		if (req_set[1])
			printf("req1: %x(%d)\n", req_set[1], req_set[1]->key);
		else
			//printf("req1: %x(NULL)\n", req_set[1]);
			printf("req1: %x(%d)\n", wb[i+1], wb[i+1]->key);
#endif
		//printf("req0: %x(%d), req1: %x(%d)\n", req_set[0], req_set[0]->key, req_set[1], req_set[1]->key);
		__block_set(req_set);

	}
	wb_empty = 1;
	for (int i=0; i<WB_SIZE; ++i)
		wb[i] = NULL;

	//exit(0);
	
	/*
	문제!! 0, 1, 1, 1, 2, 2. 이렇게 된 건 어떻게 처리?
		차라리 처음에 바로 심화된 same lpa check를 하는 게 낫지 않을까? 
		check 한번 하고, 두번째거와 세번째것도 check하는 식으로.. 다른 게 나올 때까지.
		근데 보통의 경우에는 overhead가 클 텐데..
		1, 1이면 기존 테스트로도 걸러진다. 0, 1, 1이면 이게 compaction 되는 경우에만 그 다음것도 검사하면 되지 않을까?
		즉, all pass인 경우에는 2번째것과 그 이후것을 비교. 다른 게 나올때까지 계속 뒤로 가서.. 그걸로 req_set[1]을 채우고
		그에 맞게 i도 ++해주기. 
		그리고, compaction 성공했으면 i++ 해주어야.. 원래도.
	*/




#if 0
	// 여기서 두개씩 합쳐서 set으로 넣어야겠네.
	for (int i=0; i<WB_SIZE/RPP; ++i) {
		for (int j=0; j<RPP; ++j) {
			req_set[j] = wb[i*RPP + j];
		}
		//__block_set(req_set);
	}
#endif
	
	// compaction해야 한다.
	// 1. old request 버리기
	// 2. clustering
	// clustering과 같이 진행되야 할 것 같다.

	/* 어떻게 clustering 할까?
	   일단 sorting이 필요할 것 같다.
	   아니, 애초에 buffer에 하나씩 넣을 때마다 table에 buffer의 내용물을 모두
	   저장해 놓으면.. full에서 꺼낼 때.. 아니 처음에 넣을 때 바로바로 짝꿍이
	   있는지 파악할 수 있을 것이다.
	   그런데 buffer table이 가능한가? lpa 만큼 table이 있어야 할 것 같은데.
	   lpa를 index로 하지 말고, 그냥 넣는 순서대로 저장해놓고.. 근데 그렇게 하면 
	   buffer 크기 1024 기준으로 하나 합치려고 최대 1024짜리를 search해야 한다.
	   lpa에 대한 Hash table로 만드는 게 가장 합리적인 것 같다. 
	   처음에 넣을 때, 일단 lpa/ppb로 lba를 구하고, 음.. 애초에 lba가 같은 것들
	   끼리 묶어서? 근데 그래도 block 개수만큼 table이 생긴다. 
	   hash로 한다고 해도.. lpa의 범위 자체가 nop만큼 크기 때문에 hash table을 
	   엄청 크게 만들 게 아니라면 collision이 너무 많이 생겨있다. 그건 별로
	   좋지 못하다.
	   그나마 가능한 게 lba 기준으로 묶어서 저장해놓고 그 안에서 되는 것들만 
	   체크해놔서 묶거나, 혹은 그냥 2개씩 내려보낼 때마다 일일이 buffer 전체를
	   search해서 짝꿍이 있는지 검색하는 것. buffer를 struct로 해서 
	   그냥 request array가 아니라 request, lba, offset/2를 저장하게 함으로써
	   lba가 같고 offset/2가 같은지 하나씩 검사하면서 찾기. 단순한 O(n^2) 형태로
	   구현이 가능할 것. 이게 가장 현실성있어 보인다. 

	   아니면, sort한 다음, entry마다 그 '다음 번째 entry'가 자신과 짝꿍인지 검사
	   하는 형태로 하는 것. 그 다음 번째 entry가 자신과 lba와 offset/2가 같은 지
	   검사하게 된다면, 실질적으로 짝꿍을 찾는 과정은 overhead가 적을 것이다.
	   그렇다면, full 상태에서 buffer의 모든 entry들을 sorting한 뒤 짝꿍을 
	   순차적으로 찾는 이 방식으로 하자.
	   sorting은 어떻게? 꽉 찼을 때만 sort 상태가 필요하고 그 상태에서는 buffer를
	   완전히 비우므로, 굳이 tree로 유지하고 있을 필요는 없을 것이다.
	   따라서, request들을 그 lpa 기준으로 quick sort로 정렬하는 것이 좋을 것이다.

	   방법 1. full에서 stable sort를 한번 해서 flush하기. 이 방법은 write 자체는
	   잘 처리하지만, read할 때마다 buffer를 O(n)만큼 linear search해야 한다.
	   방법 2. buffer 자체를 skip list로 관리하기. sorting에 필요한 총 overhead는
	   방법 1보다 좀 더 크겠지만, 이 경우 read할 때 하는 search가 O(logn)이 되기 
	   때문에 read hit까지 고려하면 이 편이 더 낫다. 구현은 더 복잡.
	 */

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


