#include "block.h"

// BT부터 잘못된 것 같다. get 할 때 PBA 결과가 이상하다.
void GC_moving(request **const req_set, algo_req* my_req, 
    uint32_t LBA, uint32_t offset, uint32_t old_PBA, uint32_t old_PPA)
{
	uint32_t old_PPA_zero;
    uint32_t PSA;
	uint32_t numValid;
	uint32_t new_PBA;
	uint32_t new_PPA_zero;
	uint32_t key_offset;
	value_set** temp_set;
	block_sram* sram_table;
	segment_table* seg_;

	old_PPA_zero = old_PBA * ppb_;
	//printf("GC_moving start!\n");
    
	//printf("GC_moving start!\n");
    if (!(BT[LBA].alloc_block = BM_Dequeue(bQueue))) {
		printf("in GC_moving, GC start!\n");
        block_GC();
        BT[LBA].alloc_block = BM_Dequeue(bQueue);
		if (!BT[LBA].alloc_block) {
			while(1) {
				printf("Dequeue.. No return!!!\n");
			}
		}
    }
#if 1
	printf("BT[%d].%d\n", LBA, BT[LBA].PBA);
	printf("BT[%d].alloc_block = %x\n", LBA, BT[LBA].alloc_block);
	printf("BT[%d].alloc_block->PBA = %d\n", LBA, BT[LBA].alloc_block->PBA);
#endif
    BT[LBA].PBA = BT[LBA].alloc_block->PBA; // new PBA
	new_PBA = BT[LBA].PBA;
	PSA = LBA_TO_PSA(BT, LBA); // BT[LBA].PBA / bps_ // 지금 이거 안쓰는중
	new_PPA_zero = new_PBA * ppb_;
	seg_ = &ST[BT[LBA].PBA/bps_];
	if (!seg_->segblock.hn_ptr)
		seg_->segblock.hn_ptr = BM_Heap_Insert(bHeap, &seg_->segblock);
#if 0
    if (ST[PSA].segblock.hn_ptr == NULL) {
        ST[PSA].segblock.hn_ptr = BM_Heap_Insert(bHeap, &ST[PSA].segblock);
    }
#endif
	printf("BT[LBA(%d)].PBA = %d\n", LBA, BT[LBA].PBA);
	printf("PSA: %d\n", PSA); // bps 변화 때문에.. 2 나눠야 할 수도
	//PSA /= 2;

    
	for (int i=0; i<nob_; i++) {
		if (BT[i].PBA > 1000000000) {
			printf("Too big PBA in BT!!\n");
			exit(1);
		}
	}
    numLoaded = 0;
    numValid = 0;
	temp_set = (value_set**)malloc(sizeof(value_set*) * ppb_);
	sram_table = (block_sram*)malloc(sizeof(block_sram) * ppb_);
	
	for (int i=0; i<ppb_; ++i) {
		sram_table[i].SRAM_OOB.LPA = NIL;
		sram_table[i].SRAM_OOB.LPA2 = NIL;
		sram_table[i].SRAM_PTR= NULL;
		temp_set[i] = NULL;
	}

	// SRAM_load
	int offcase = 0;
	for (int i=0; i<ppb_; ++i) {
		if (i == (int32_t)offset) {
			sram_table[numValid].SRAM_PTR = (PTR)malloc(PAGESIZE);
			sram_table[numValid].SRAM_OOB.LPA = LBA * (ppb_*2) + (offset*2); // (LBA, PBA 0 기준으로)offset이 1이면, LPA는 2와 3일 것.
			if (req_set[1])
				sram_table[numValid].SRAM_OOB.LPA2 = LBA * (ppb_*2) + (offset*2)+1;  // 이게 필요한지는 아직 모르겠다.
			else
				sram_table[numValid].SRAM_OOB.LPA2 = (uint32_t)NIL;
			numValid++;
			offcase = 1;
		}
		else if (BM_IsValidPage(BM, old_PPA_zero + i)) {
			temp_set[numValid] = SRAM_load(sram_table, i + old_PPA_zero, numValid);
			numValid++;
		}
	}
	if (offcase == 0)
		while(1) printf("off??");
	
	// Waiting Loading by Polling
	//printf("numLoaded: %d, numValid: %d\n", numLoaded, numValid);
	while (numLoaded != numValid - 1) {}
	//printf("numLoaded: %d, numValid: %d\n", numLoaded, numValid);

	// memcpy values from value_set
	for (int i=0; i<(int32_t)numValid; ++i) {
		if (!temp_set[i]) {
			if (req_set[1]) {
				memcpy(sram_table[i].SRAM_PTR, req_set[0]->value->value, PAGESIZE/2);
				memcpy(sram_table[i].SRAM_PTR + PAGESIZE/2, req_set[1]->value->value, PAGESIZE/2);
			} else {
				memcpy(sram_table[i].SRAM_PTR, req_set[0]->value->value, PAGESIZE);
			}
			
		}
		else {
			memcpy(sram_table[i].SRAM_PTR, temp_set[i]->value, PAGESIZE);
			inf_free_valueset(temp_set[i], FS_MALLOC_R);
		}
	}

	// SRAM_unload
	for (int i=0; i<(int32_t)numValid; ++i) {
		//key_offset = sram_table[i].SRAM_OOB.LPA % ppb_; // PPA offset
		key_offset = (sram_table[i].SRAM_OOB.LPA % (ppb_*2)) / 2; // PPA offset
		// lpa 2 기준으로 보면, lpa는 원래 LBA * 8 + 2(LPA offset) 일 것
		// 따라서 (LPA % (ppb_*2)) / 2 이지 않을까?
		// 이렇게 하면, lpa 7이였으면 offset 3이 되겠지.
		SRAM_unload(sram_table, key_offset + new_PPA_zero, i, req_set[1]);
	}

	free(temp_set);
	free(sram_table);

	/* Trim the block of old PPA */
	// NO. Just invalidate the 'Block'
	BM_InitializeBlock(BM, old_PBA); // bitmap 0, invalid 0
	//ST[PSA].segblock.Invalid++; // now, PSA == new_PBA/bps_
	ST[old_PBA/bps_].segblock.Invalid++;

	//__block.li->trim_block(old_PPA_zero, false);
	//BM_Enqueue(bQueue, &BM->barray[PBA]); // Invalidate만 한 셈이니 Free Queue에 넣으면 안된다
	//printf("GC_moving end!\n");
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
	segment_table* seg_;

	numGC++;
	if ((numGC == 1) || !(numGC % 30)) {
		printf("GC start!\n");
		printf("numGC: %d\n", numGC);
	}
	uint32_t victimSeg_PBA;
    victimBlock = BM_Heap_Get_Max(bHeap);
	victimSeg = &ST[victimBlock->PBA]; // 사실 여기서 victimBlock의 역할은 끝. 
	victimSeg->segblock.hn_ptr = NULL;
	victimSeg_PBA = victimSeg->segblock.PBA; // victimSeg->PBA means PSA of victim segment // 그냥 victimblock->PBA 랑 같은거 아닌가?
	// *victimBlock actually means victimSeg->segblock
	old_PPA_zero = victimBlock->PBA * pps_; // 'PPA' of victim segment

	//while (1)
		//printf("GC!");
	while (victimSeg_PBA != (uint32_t)victimBlock->PBA)
		printf("NONONO!");
	

	if (victimBlock->Invalid == bps_) { // victim segment의 모든 block이 invalid인 경우, valid page들을 옮길 필요 없이 바로 trim하고 끝내면 된다.
		printf("victimBlock pages are all invalid!\n");
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
	else if (victimBlock->Invalid == 0) { // all pages are valid
		printf("All pages are valid. There are no extra pages\n");
		exit(1);
	}

    // Fill bQueue with Reserved Segment
	// Segment에서 Block*에 접근해야 하므로 blockmap 사용해야..
    for (int i=0; i<bps_; i++) {
        BM_Enqueue(bQueue, reservedSeg->blockmap[i]);
    }

	// victim segment 중 valid page만 옮기자. 옮기는 목적지는 Dequeue해서 바로바로 얻어내고.
	numLoaded = 0;
	numValid = 0;

	sram_table = (block_sram*)malloc(sizeof(block_sram) * pps_);
	temp_set = (value_set**)malloc(sizeof(value_set*)*pps_);

	for (int i=0; i<pps_; ++i) {
		sram_table[i].SRAM_OOB.LPA = (uint32_t)NIL;
		sram_table[i].SRAM_OOB.LPA2 = (uint32_t)NIL;
		sram_table[i].SRAM_PTR= NULL;
		temp_set[i] = NULL;
	}

	// SRAM_load
	for (int i=0; i<pps_; ++i) {
		if (BM_IsValidPage(BM, old_PPA_zero+i)) {
			temp_set[numValid] = SRAM_load(sram_table, i + old_PPA_zero, numValid);
			//temp_set[i] = SRAM_load(sram_table, i, old_PPA_zero, numValid);
			numValid++;
		}
	}
	// Here, start
	// pps 기준으로 for문 돌려서, 이 아래부터는 valid 개수만큼만 for문을 돌리게 한 뒤 OOB에 적어둔 lpa로 offset을 다시 구해서 하는 게 더 연산이 적다. 일단 OOB부터 구현해야..
	// 안하면 연산이 얼마나 더 늘어날까?
	// Waiting Loading by Polling
	while (numLoaded != numValid) {}

	for (int i=0; i<(int32_t)numValid; ++i) {
		memcpy(sram_table[i].SRAM_PTR, temp_set[i]->value, PAGESIZE);
		key_LBA = sram_table[i].SRAM_OOB.LPA / (ppb_*2);
		if (BT[key_LBA].PBA != NIL) {
			temp_PBA = BT[key_LBA].PBA;
			BT[key_LBA].PBA = NIL;
			BM_InitializeBlock(BM, temp_PBA);
		}
		inf_free_valueset(temp_set[i], FS_MALLOC_R);
	}
	/*

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
	*/

#if 0
	// Initialize old blocks in Victim Segment
	for (i=0; i<bps_; ++i) {
		key_LBA = sram_table[i*ppb_].SRAM_OOB.LPA / ppb_;
		BT[key_LBA].PBA = NIL;
		BM_InitializeBlock(BM, victimSeg_PBA + i);
	}
#endif

	// SRAM_unload
	for (int i=0; i<(int32_t)numValid; ++i) {
		key_LBA = sram_table[i].SRAM_OOB.LPA / (ppb_*2);
		//key_offset = sram_table[i].SRAM_OOB.LPA % ppb_;
		key_offset = (sram_table[i].SRAM_OOB.LPA % (ppb_*2)) / 2; // PPA offset
		if (BT[key_LBA].PBA == NIL) {
			BT[key_LBA].alloc_block = BM_Dequeue(bQueue);
			BT[key_LBA].PBA = BT[key_LBA].alloc_block->PBA;
			seg_ = &ST[BT[key_LBA].PBA/bps_];
			if (!seg_->segblock.hn_ptr)
				seg_->segblock.hn_ptr = BM_Heap_Insert(bHeap, &seg_->segblock);
#if 0
			PSA = LBA_TO_PSA(BT, key_LBA);
			if (ST[PSA].segblock.hn_ptr == NULL) {
				ST[PSA].segblock.hn_ptr = BM_Heap_Insert(bHeap, &ST[PSA].segblock);
			}
#endif
		}
		SRAM_unload(sram_table, key_offset + BT[key_LBA].PBA * ppb_, i, NULL); // req1이 있는 지 알아야 하지 않을까?
	}
	/*	
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
	*/

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

