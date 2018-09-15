#include "block.h"
#include <unistd.h>

/* block ftl with SEGMENT support */
   //need GC algorithm

/* ASYNC ver */
uint32_t numLoaded;
int32_t nob_;
int32_t nos_;
int32_t ppb_;
int32_t pps_;
int32_t bps_;
BM_T* BM;
b_queue* bQueue;
Heap* bHeap;
block_table* BT;
segment_table* ST;
segment_table* reservedSeg;
block_OOB* OOB;

struct algorithm __block={
	.create=block_create,
	.destroy=block_destroy,
	.get=block_get,
	.set=block_set,
	.remove=block_remove
};

/* Check Last offset */
int8_t block_CheckLastOffset(uint32_t LBA, uint32_t offset)
{
	if (BT[LBA].lastoffset > offset)
		return 0; // Bad case
	return 1; // Good case
#if 0
	if (lastoffset_array[PBA] <= offset) {
		lastoffset_array[PBA] = offset;
		return 1; // good case
	}
	else // Moving Block with target offset update
		return 0;
#endif
}

uint32_t block_create (lower_info* li,algorithm *algo){
	printf("block_create start!\n");
	algo->li=li;
	nob_ = _NOB;
	nos_ = _NOS;
	ppb_ = _PPB;
	pps_ = _PPS;
	bps_ = _BPS;
	BM = BM_Init(nob_, ppb_, 1, 1); // 1 FreeQueue

	BT = (block_table*)malloc(sizeof(block_table) * nob_);
	ST = (segment_table*)malloc(sizeof(segment_table) * nos_);
	OOB = (block_OOB*)malloc(sizeof(block_OOB) * nop_);

	for (int i=0; i<nob_; i++) {
		BT[i].PBA = NIL;
		BT[i].lastoffset = 0;
		BT[i].valid = 1;
		alloc_block = NULL;
	}
	for (int i=0; i<nos_ i++) {
		ST[i].segblock.PBA = i; // PSA(Physical Segment Address)
		ST[i].segblock.Invalid = 0; // number of Invalid blocks in Segment i
		ST[i].segblock.hn_ptr = NULL;
		ST[i].segblock.type = 0; // meaningless
		ST[i].segblock.ValidP = NULL; // meaningless
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
	for (int i=0; i<(nob_-bps_); ++i)
		BM_Enqueue(bQueue, &BM->barray[i]);
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
	for (int i=0; i<nos_; ++i) {
		free(ST[i].blockmap);
	free(ST);
	free(OOB);

	BM_Free(BM);
}
uint32_t block_get(request *const req){
	uint32_t LBA;
	uint32_t offset;
	uint32_t PBA;
	uint32_t PPA;
	algo_req* my_req;

	bench_algo_start(req);

	/* Request production */
	my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req=block_end_req;

	LBA = req->key / ppb_;
	offset = req->key % ppb_;
	PBA = BT[LBA].PBA;
	PPA = PBA * ppb_ + offset;

	if (!BM_IsValidPage(BM, PPA)) {
		printf("\nRead Empty Page..\n");
		bench_algo_end(req);
		return 1;
	}

	bench_algo_end(req);
	__block.li->pull_data(PPA, PAGESIZE, req->value, ASYNC, my_req);

	return 0;
}

uint32_t block_set(request *const req){
	uint32_t LBA;
	uint32_t offset;
	uint32_t PBA;
	uint32_t PPA;
	algo_req* my_req;

	bench_algo_start(req);

	/* Request production */
	my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = block_end_req;

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
		uint32_t PSA = LBA_TO_PSA(LBA);
		if (ST[PSA].segblock.hn_ptr == NULL) {
			ST[PSA].segblock.hn_ptr = BM_Heap_Insert(bHeap, &ST[PSA].segblock);
		}
	}
	PBA = BT[LBA].PBA;
	PPA = PBA * ppb_ + offset;
	if (!block_CheckLastOffset(LBA, offset) || BM_IsValidPage(BM, PPA)) {
		// Bad ASC order or Page Collision occurs
		GC_moving();
		bench_algo_end(req);
	}
	else {
		// case: We can write the page directly
		BM_ValidatePage(BM, PPA);
		OOB[PPA].LPA = req->key; // save LPA
		BT[LBA].lastoffset = offset;
		bench_algo_end(req);
		__block.li->push_data(PPA, PAGESIZE, req->value, ASYNC, my_req);
	}

	return 0;
//
	if (block_maptable[LBA] == NIL)
	{
		set_pointer = BM_Dequeue(bQueue)->PBA;
		if (set_pointer == (uint32_t)-1) while(1) printf("WHAT!");

		// Switch ERASE to EXIST of block_valid_array
		block_valid_array[set_pointer] = EXIST;

		// Write PBA of mapping result in maptable
		block_maptable[LBA] = set_pointer;

		PPA = set_pointer * ppb_ + offset; // Equal to above 2 lines

		if (BM_IsValidPage(BM, PPA))
			while(1)
				printf("ERROROROROROR!\n");
		BM_ValidatePage(BM, PPA);

		// write
		bench_algo_end(req);
		__block.li->push_data(PPA, PAGESIZE, req->value, ASYNC, my_req);
	}

	else
	{
		PBA = block_maptable[LBA];
		PPA = PBA  * ppb_ + offset;

		if (!BM_IsValidPage(BM, PPA))
		{
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
			GC_moving(req, my_req, LBA, offset, PBA, PPA, checker);
		}
		else
			while(1) printf("Valid???");
	}
	return 0;

}
uint32_t block_remove(request *const req){
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

value_set* SRAM_load(PTR* sram_value, uint32_t i, uint32_t old_PPA_zero)
{
	algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	temp_req->parents = NULL;
	temp_req->end_req = block_algo_end_req;
	__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
	sram_value[i] = (PTR)malloc(PAGESIZE);
	return temp_value_set;
	// sram malloc PAGESIZE 여기서. 인수로 sram 받아서 할당하고 unload에서 free.
}

void SRAM_unload(PTR* sram_value, uint32_t i, uint32_t new_PPA_zero)
{
	algo_req* temp_req2 = (algo_req*)malloc(sizeof(algo_req));
	//value_set* temp_value_set2 = inf_get_valueset(sram_valueset[i].value, FS_MALLOC_W, PAGESIZE);
	value_set* temp_value_set2 = inf_get_valueset(sram_value[i], FS_MALLOC_W, PAGESIZE);
	temp_req2->parents = NULL;
	temp_req2->end_req = block_algo_end_req;
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, temp_req2);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
	free(sram_value[i]);
}
void SRAM_unload_target(PTR* sram_value, uint32_t i, uint32_t new_PPA_zero, algo_req* my_req)
{
	value_set* temp_value_set2 = inf_get_valueset(sram_value[i], FS_MALLOC_W, PAGESIZE);
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, my_req);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
	free(sram_value[i]);
}

void GC_moving(request *const req, algo_req* my_req, uint32_t LBA, uint32_t offset, uint32_t PBA, uint32_t PPA, int8_t checker)
{
	/* Allocate an empty block */
	set_pointer = BM_Dequeue(bQueue)->PBA;

	/* Maptable updates for data moving */
	block_maptable[LBA] = set_pointer;
	block_valid_array[set_pointer] = EXIST;
	block_valid_array[PBA] = ERASE; // PBA means old_PBA

	uint32_t old_PPA_zero = PBA * ppb_;
	uint32_t new_PBA = block_maptable[LBA];
	uint32_t new_PPA_zero = new_PBA * ppb_;
	uint32_t numValid = 0;
	uint32_t i = 0;

	/* Start move */

	numLoaded = 0;
	value_set** temp_set = (value_set**)malloc(sizeof(value_set*)*ppb_);
	sram_value = (PTR*)malloc(sizeof(PTR) * ppb_);
	for (i=0; i<ppb_; ++i)
		sram_value[i] = NULL;

	// SRAM_load
	for (i=0; i<ppb_; ++i) {
		if (BM_IsValidPage(BM, old_PPA_zero+i)) {
			if (i != offset) {
				numValid++; // The name is numValid, but it excludes the target offset.
				temp_set[i] = SRAM_load(sram_value, i, old_PPA_zero);
			}
		}
	}

	// Waiting Loading by Polling
	while (numLoaded != numValid) {}

	// memcpy values from value_set
	for (i=0; i<ppb_; ++i) {
		if (BM_IsValidPage(BM, old_PPA_zero+i)) {
			if (i != offset) {
				memcpy(sram_value[i], temp_set[i]->value, PAGESIZE);
				inf_free_valueset(temp_set[i], FS_MALLOC_R);
			}
		}
	}
	sram_value[offset] = (PTR)malloc(PAGESIZE);
	memcpy(sram_value[offset], req->value->value, PAGESIZE);

	// SRAM_unload
	for (i=0; i<ppb_; ++i) {
		BM_InvalidatePage(BM, old_PPA_zero + i);
		if (sram_value[i]) {
			if (i != offset)
				SRAM_unload(sram_value, i, new_PPA_zero);
			else 
				SRAM_unload_target(sram_value, i, new_PPA_zero, my_req);

			BM_ValidatePage(BM, new_PPA_zero + i);
		}
	}

	free(sram_value);

	/* Trim the block of old PPA */
	__block.li->trim_block(old_PPA_zero, false);
	BM_Enqueue(bQueue, &BM->barray[PBA]);
	bench_algo_end(req);
}
