#include "block.h"

void GC_moving(value_set *data, int32_t lba, int32_t offset){
	int32_t new_pba;
	int32_t new_ppa;
	int32_t t_offset;
	int32_t old_pba;
	int32_t old_ppa;
	int valid_page_num;
	seg_status *seg_;
	value_set **temp_set;
	block_sram *d_sram;
	
	old_pba = BS[lba].pba;
	old_ppa = old_pba * ppb_;

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
	new_pba = BS[lba].pba;
	new_ppa = new_pba * ppb_;

	valid_page_num = 0;
	numLoaded = 0;
	d_sram = (block_sram*)malloc(sizeof(block_sram) * ppb_);
	temp_set = (value_set**)malloc(sizeof(value_set*) * ppb_);

	for(int i = 0; i < ppb_; i++){
		d_sram[i].PTR_RAM = NULL;
		d_sram[i].OOB_RAM.lpa = -1;
		temp_set[i] = NULL;
	}

	/* read valid pages in block */
	for(int i = 0; i < ppb_; i++){
		if(i == offset){
			d_sram[valid_page_num].PTR_RAM = (PTR)malloc(PAGESIZE);
			d_sram[valid_page_num].OOB_RAM.lpa = lba * ppb_ + offset;
			valid_page_num++;
		}
		else if(BM_IsValidPage(BM, old_ppa + i)){
			temp_set[valid_page_num] = SRAM_load(d_sram, old_ppa + i, valid_page_num);
			valid_page_num++;
		}
	}

	while (numLoaded != valid_page_num - 1) {}

	for(int i = 0; i < valid_page_num; i++){
		if(!temp_set[i]){
			memcpy(d_sram[i].PTR_RAM, data->value, PAGESIZE);
		}
		else{
			memcpy(d_sram[i].PTR_RAM, temp_set[i]->value, PAGESIZE);
			inf_free_valueset(temp_set[i], FS_MALLOC_R);
		}
	}

	for(int i = 0; i < valid_page_num; i++){
		t_offset = d_sram[i].OOB_RAM.lpa % ppb_;
		SRAM_unload(d_sram, new_ppa + t_offset, i);
	}

	free(temp_set);
	free(d_sram);

	BM_InitializeBlock(BM, old_pba);
	SS[old_pba/bps_].seg.Invalid++;

	return ;
}

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