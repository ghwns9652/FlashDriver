#include "block.h"
#include "../../bench/bench.h"

struct algorithm __block={
	.create=block_create,
	.destroy=block_destroy,
	.get=block_get,
	.set=block_set,
	.remove=block_remove
};

/* ASYNC ver */
int32_t nop_;
int32_t nob_;
int32_t ppb_;
int32_t nos_;
int32_t pps_;
int32_t bps_;
int32_t numLoaded;
BM_T *BM;
b_queue *free_b;
Heap *b_heap;
seg_status *SS;
seg_status *reserved;
block_status* BS;
B_OOB* block_oob;


uint32_t block_create (lower_info* li,algorithm *algo){
	printf("block_create start!\n");
	
	nop_ = _NOP;
	nob_ = _NOB;
	ppb_ = _PPB;
	nos_ = _NOS;
	pps_ = _PPS;
	bps_ = BPS;

	printf("async: %d\n", ASYNC);
	printf("# of page: %d\n", nop_);
	printf("# of block: %d\n", nob_);
	printf("page per block: %d\n", ppb_);
	printf("# of segment: %d\n", nos_);
	printf("page per segment: %d\n", pps_);
	printf("block per segment: %d\n", bps_);

	algo->li=li;
	BS = (block_status*)malloc(sizeof(block_status) * nob_);
	SS = (seg_status*)malloc(sizeof(seg_status) * nos_);
	block_oob = (B_OOB*)malloc(sizeof(B_OOB) * nop_);
	for(int i = 0; i < nob_; i++){
		BS[i].pba = -1;
		BS[i].last_offset = 0;
		BS[i].alloc_block = NULL;
	}

	for(int i = 0; i < nop_; i++){
		block_oob[i].lpa = -1;
	}

	BM = BM_Init(nob_, ppb_, 1, 1); // init에 블록 개수 설정가능하게 바꿔야될꺼같음

	for(int i = 0; i < nos_; i++){
		SS[i].seg.PBA = i;
		SS[i].seg.Invalid = 0;
		SS[i].seg.hn_ptr = NULL;
		SS[i].block = (Block**)malloc(sizeof(Block*) * bps_);
		for(int j = 0; j < bps_; j++){
			SS[i].block[j] = &BM->barray[i * bps_ + j];
		}
	}
	reserved = &SS[nos_ - 1];

	BM_Queue_Init(&free_b);
	for(int i = 0; i < nos_ - 1; i++){
		for(int j = 0; j < bps_; j++){
			BM_Enqueue(free_b, &BM->barray[i * bps_ + j]);
		}
	}
	b_heap = BM_Heap_Init(nos_ - 1);

	BM->harray[0] = b_heap;
	BM->qarray[0] = free_b;

	printf("block_create end!\n");
	return 0;
}

void block_destroy (lower_info* li, algorithm *algo){
	BM_Free(BM);
	for(int i = 0; i < bps_; i++){
		free(SS[i].block);
	}
	free(SS);
	free(BS);
	free(block_oob);
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
	pba = BS[lba].pba;
	ppa = pba * ppb_ + offset;

	if(!BM_IsValidPage(BM, ppa)){
		bench_algo_end(req);
		req->type = FS_NOTFOUND_T;
		req->end_req(req);
		return 1;
	}
	bench_algo_end(req);
	__block.li->pull_data(ppa, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_R, NULL, req));
	return 0;
}

uint32_t block_set(request *const req){
	int32_t lba;
	int32_t lpa;
	int32_t pba;
	int32_t ppa;
	int32_t offset;
	seg_status *seg_;

	bench_algo_start(req);
	lpa = req->key;
	lba = lpa / ppb_;
	offset = lpa % ppb_;
	if(BS[lba].pba == -1){
		BS[lba].alloc_block = BM_Dequeue(free_b);
		if(BS[lba].alloc_block == NULL){
			block_GC();
			BS[lba].alloc_block = BM_Dequeue(free_b);
		}

		BS[lba].pba = BS[lba].alloc_block->PBA;
		seg_ = &SS[BS[lba].pba/bps_];
		if(!seg_->seg.hn_ptr){
			seg_->seg.hn_ptr = BM_Heap_Insert(b_heap, &seg_->seg);
		}
	}
	pba = BS[lba].pba;
	ppa = pba * ppb_ + offset;
	if(!block_CheckLastOffset(lba, offset) || BM_IsValidPage(BM, ppa)){
		GC_moving(req->value, lba, offset);
		bench_algo_end(req);
		req->end_req(req);
	}
	else{
		bench_algo_end(req);
		__block.li->push_data(ppa, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_W, NULL, req));
		BS[lba].last_offset = offset;
		BM_ValidatePage(BM, ppa);
		block_oob[ppa].lpa = lpa;
	}
	return 0;
}

uint32_t block_remove(request *const req){
	//	block->li->trim_block()
	return 0;
}
