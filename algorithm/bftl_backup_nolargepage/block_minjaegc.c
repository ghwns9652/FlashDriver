#include "block.h"

void GC_moving(request *const req, algo_req* my_req, 
    uint32_t LBA, uint32_t offset, uint32_t old_PBA, uint32_t old_PPA)
{
	uint32_t old_PPA_zero;
    uint32_t PSA;
	uint32_t numValid;
	uint32_t new_PBA;
	uint32_t new_PPA_zero;
	value_set** temp_set;
	block_sram* sram_table;

	old_PPA_zero = old_PBA * ppb_;
	printf("GC_moving start!\n");
    
    if (!(BT[LBA].alloc_block = BM_Dequeue(bQueue))) {
        block_GC();
        BT[LBA].alloc_block = BM_Dequeue(bQueue);
    }
    BT[LBA].PBA = BT[LBA].alloc_block->PBA; // new PBA
	new_PBA = BT[LBA].PBA;
	PSA = LBA_TO_PSA(BT, LBA);
	new_PPA_zero = new_PBA * ppb_;
    if (ST[PSA].segblock.hn_ptr == NULL) {
        ST[PSA].segblock.hn_ptr = BM_Heap_Insert(bHeap, &ST[PSA].segblock);
    }
    
    numLoaded = 0;
    numValid = 0;
	temp_set = (value_set**)malloc(sizeof(value_set*) * ppb_);
	sram_table = (block_sram*)malloc(sizeof(block_sram) * ppb_);
	
	for (int i=0; i<ppb_; ++i) {
		sram_table[i].SRAM_OOB.LPA = NIL;
		sram_table[i].SRAM_PTR= NULL;
		temp_set[i] = NULL;
	}

	// SRAM_load
	for (int i=0; i<ppb_; ++i) {
		if (BM_IsValidPage(BM, old_PPA_zero+i)) {
			if (i != (int32_t)offset) {
				numValid++; // The name is numValid, but it excludes the target offset.
				temp_set[i] = SRAM_load(sram_table, i, old_PPA_zero);
			}
		}
	}
	// Here, start
	// Waiting Loading by Polling
	while (numLoaded != numValid) {}

	// memcpy values from value_set
	for (int i=0; i<ppb_; ++i) {
		if (BM_IsValidPage(BM, old_PPA_zero+i)) {
			if (i != (int32_t)offset) {
				memcpy(sram_table[i].SRAM_PTR, temp_set[i]->value, PAGESIZE);
				inf_free_valueset(temp_set[i], FS_MALLOC_R);
			}
		}
	}
	//sram_value[offset] = (PTR)malloc(PAGESIZE);
	sram_table[offset].SRAM_PTR = (PTR)malloc(PAGESIZE);
	//memcpy(sram_value[offset], req->value->value, PAGESIZE);
	memcpy(sram_table[offset].SRAM_PTR, req->value->value, PAGESIZE);

	// SRAM_unload
	for (int i=0; i<ppb_; ++i) {
		//BM_InvalidatePage(BM, old_PPA_zero + i);
		if (sram_table[i].SRAM_PTR) {
			if (i != (int32_t)offset)
				SRAM_unload(sram_table, i, new_PPA_zero);
			else 
				SRAM_unload_target(sram_table, i, new_PPA_zero, my_req);

			//BM_ValidatePage(BM, new_PPA_zero + i);
		}
	}
	free(temp_set);
	free(sram_table);

	/* Trim the block of old PPA */
	// NO. Just invalidate the 'Block'
	BM_InitializeBlock(BM, old_PBA); // bitmap 0, invalid 0
	ST[PSA].segblock.Invalid++;

	//__block.li->trim_block(old_PPA_zero, false);
	//BM_Enqueue(bQueue, &BM->barray[PBA]); // Invalidate만 한 셈이니 Free Queue에 넣으면 안된다
}




void block_GC()
{
    Block* victimBlock; // block as a victim segment
	segment_table* victimSeg; // victim segment
	PBA_T victimBlock_PBA;
	int8_t all_block_invalid = 0;
	value_set** temp_set;
	block_sram* sram_table;
	uint32_t old_PPA_zero;
	uint32_t numValid;
	uint32_t key_LBA;
	uint32_t key_offset;
	uint32_t temp_PBA;
	uint32_t PSA;

	printf("block_GC start!\n");
	uint32_t victimSeg_PBA;
    victimBlock = BM_Heap_Get_Max(bHeap);
	victimSeg = &ST[victimBlock->PBA]; // 사실 여기서 victimBlock의 역할은 끝. 
	victimSeg->segblock.hn_ptr = NULL;
	victimSeg_PBA = victimSeg->segblock.PBA; // victimSeg->PBA means PSA of victim segment // 그냥 victimblock->PBA 랑 같은거 아닌가?
	// *victimBlock actually means victimSeg->segblock
	old_PPA_zero = victimBlock->PBA * pps_; // 'PPA' of victim segment

	while (1)
		printf("GC!");
	while (victimSeg_PBA != (uint32_t)victimBlock->PBA)
		printf("NONONO!");
	
    // Fill bQueue with Reserved Segment
	// Segment에서 Block*에 접근해야 하므로 blockmap 사용해야..
    for (int i=0; i<bps_; i++) {
        BM_Enqueue(bQueue, reservedSeg->blockmap[i]);
    }

	if (victimBlock->Invalid == bps_) { // victim segment의 모든 block이 invalid인 경우, valid page들을 옮길 필요 없이 바로 trim하고 끝내면 된다.
		all_block_invalid = 1;
		//victimBlock->Invalid = 0;
		__block.li->trim_block(old_PPA_zero, false);
		reservedSeg = victimSeg;
		reservedSeg->segblock.Invalid = 0;
		// hn_ptr은 위에서 미리 NULL 처리됨
		// InitializeBlock 할 필요는 없을까?
		for (int i=0; i<bps_; ++i) 
			BM_InitializeBlock(BM, reservedSeg->blockmap[i]->PBA);
		return ;
	}

	// victim segment 중 valid page만 옮기자. 옮기는 목적지는 Dequeue해서 바로바로 얻어내고.
	numLoaded = 0;
	numValid = 0;

	temp_set = (value_set**)malloc(sizeof(value_set*)*pps_);
	sram_table = (block_sram*)malloc(sizeof(block_sram) * pps_);
	for (int i=0; i<pps_; ++i) {
		sram_table[i].SRAM_OOB.LPA = NIL;
		sram_table[i].SRAM_PTR= NULL;
		temp_set[i] = NULL;
	}

	// SRAM_load
	for (int i=0; i<pps_; ++i) {
		if (BM_IsValidPage(BM, old_PPA_zero+i)) {
			numValid++; // The name is numValid, but it excludes the target offset.
			temp_set[i] = SRAM_load(sram_table, i, old_PPA_zero);
		}
	}
	// Here, start
	// pps 기준으로 for문 돌려서, 이 아래부터는 valid 개수만큼만 for문을 돌리게 한 뒤 OOB에 적어둔 lpa로 offset을 다시 구해서 하는 게 더 연산이 적다. 일단 OOB부터 구현해야..
	// 안하면 연산이 얼마나 더 늘어날까?
	// Waiting Loading by Polling
	while (numLoaded != numValid) {}

	// memcpy values from value_set
	for (int i=0; i<pps_; ++i) {
		if (BM_IsValidPage(BM, old_PPA_zero+i)) {
			memcpy(sram_table[i].SRAM_PTR, temp_set[i]->value, PAGESIZE);
			inf_free_valueset(temp_set[i], FS_MALLOC_R);
		}
	}
	// Initialize Block Table corresponding to LBA
	for (int i=0; i<pps_; i+=ppb_) {
		key_LBA = sram_table[i].SRAM_OOB.LPA / ppb_;
		//key_offset = sram_table[i].SRAM_OOB.LPA % ppb_;
		if (BT[key_LBA].PBA != NIL) {
			temp_PBA = BT[key_LBA].PBA;
			BT[key_LBA].PBA = NIL;
			BM_InitializeBlock(BM, temp_PBA);
		}
	}

#if 0
	// Initialize old blocks in Victim Segment
	for (i=0; i<bps_; ++i) {
		key_LBA = sram_table[i*ppb_].SRAM_OOB.LPA / ppb_;
		BT[key_LBA].PBA = NIL;
		BM_InitializeBlock(BM, victimSeg_PBA + i);
	}
#endif

	// SRAM_unload
	for (int i=0; i<pps_; ++i) {
		if (sram_table[i].SRAM_PTR) {
			key_LBA = sram_table[i].SRAM_OOB.LPA / ppb_;
			key_offset = sram_table[i].SRAM_OOB.LPA % ppb_;

			if (BT[key_LBA].PBA == NIL) {
				BT[key_LBA].alloc_block = BM_Dequeue(bQueue);
				BT[key_LBA].PBA = BT[key_LBA].alloc_block->PBA;
				PSA = LBA_TO_PSA(BT, key_LBA);
				if (ST[PSA].segblock.hn_ptr == NULL) {
					ST[PSA].segblock.hn_ptr = BM_Heap_Insert(bHeap, &ST[PSA].segblock);
				}
			}
			SRAM_unload(sram_table, i, BT[key_LBA].PBA * ppb_);
		}
	}

	free(temp_set);
	free(sram_table);
	__block.li->trim_block(old_PPA_zero, false);
	reservedSeg = victimSeg;
	reservedSeg->segblock.Invalid = 0;
#if 0

			temp_old_PBA = victimSeg_PBA + (i / ppb_);
			offset = i % ppb_;

			SRAM_unload(sram_table, 
			if (temp_old_PBA == NIL) {
				// 결국은 OOB가 꼭 필요하다는 것을 알았다. DGIBOX 했을 때도, GC할 때는 원래의 key(LPA) 값을 알아야만 했으므로 keyArray에다가 저장해 놨었는데, 여기도 GC하므로 마찬가지로 필요하다.


	for (i=0; i<pps_; ++i) {
		BM_InvalidatePage(BM, old_PPA_zero + i);
		if (sram_value[i]) {
			if (i != offset)
				SRAM_unload(sram_value, i, new_PPA_zero);
			else 
				SRAM_unload_target(sram_value, i, new_PPA_zero, my_req);

			//BM_ValidatePage(BM, new_PPA_zero + i);
		}
	}
#endif
}

//incomplete version
#if 0
void block_GC(){
	uint8_t all;
	int32_t t_offset;
	int32_t t_lba;
	int32_t t_pba;
	int32_t old_ppa;
	int valid_page_num;
	Block* victim;
	seg_status* seg_victim;
	seg_status *seg_;
	value_set **temp_set;
	block_sram *d_sram;

	all = 0;
	victim = BM_Heap_Get_Max(b_heap);
	seg_victim = &SS[victim->PBA];
	seg_victim->seg.hn_ptr = NULL;
	if(victim->Invalid == bps_){ // every page is invalid.
		all = 1;
	}
	else if(victim->Invalid == 0){// ssd all valid.
		printf("full!!!\n");
		exit(1);
	}

	old_ppa = seg_victim->seg.PBA * pps_;
	for(int i = 0; i < bps_; i++){
		BM_Enqueue(free_b, reserved->block[i]);
	}
	reserved = seg_victim;
	if(all){
		seg_victim->seg.Invalid = 0;
		__block.li->trim_block(old_ppa, false);
		return ;
	}
	valid_page_num = 0;
	numLoaded = 0;
	d_sram = (block_sram*)malloc(sizeof(block_sram) * pps_);
	temp_set = (value_set**)malloc(sizeof(value_set*) * pps_);

	for(int i = 0; i < ppb_; i++){
		d_sram[i].PTR_RAM = NULL;
		d_sram[i].OOB_RAM.lpa = -1;
		temp_set[i] = NULL;
	}

	/* read valid pages in block */
	for(int i = 0; i < pps_; i++){
		if(BM_IsValidPage(BM, old_ppa + i)){
			temp_set[valid_page_num] = SRAM_load(d_sram, old_ppa + i, valid_page_num);
			valid_page_num++;
		}
	}

	while (numLoaded != valid_page_num) {}

	for(int i = 0; i < valid_page_num; i++){
		memcpy(d_sram[i].PTR_RAM, temp_set[i]->value, PAGESIZE);
		t_lba = d_sram[i].OOB_RAM.lpa / ppb_;
		if((t_pba = BS[t_lba].pba) != -1){
			BS[t_lba].pba = -1;
			BM_InitializeBlock(BM, t_pba);
		}
		inf_free_valueset(temp_set[i], FS_MALLOC_R);
	}

	for(int i = 0; i < valid_page_num; i++){
		t_lba = d_sram[i].OOB_RAM.lpa / ppb_;
		t_offset = d_sram[i].OOB_RAM.lpa % ppb_;
		if(BS[t_lba].pba == -1){
			BS[t_lba].alloc_block = BM_Dequeue(free_b);
			BS[t_lba].pba = BS[t_lba].alloc_block->PBA;
			seg_ = &SS[BS[t_lba].pba/bps_];
			if(!seg_->seg.hn_ptr){
				seg_->seg.hn_ptr = BM_Heap_Insert(b_heap, &seg_->seg);
			}
		}
		SRAM_unload(d_sram, BS[t_lba].pba * ppb_ + t_offset, i);
	}

	free(temp_set);
	free(d_sram);

	seg_victim->seg.Invalid = 0;

	/* Trim block */
	__block.li->trim_block(old_ppa, false);
	return ;
}
#endif
