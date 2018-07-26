#include "block.h"

void GC_moving(value_set *data, int32_t lba, int32_t offset){
	int32_t new_pba;
	int32_t new_ppa;
	int32_t t_offset;
	int32_t old_pba;
	int32_t old_ppa;
	int valid_page_num;
	Block* old_block;
	value_set **temp_set;
	block_sram *d_sram;
	
	old_block = BS[lba].alloc_block;
	old_pba = BS[lba].pba;
	old_ppa = old_pba * ppb_;

	BS[lba].alloc_block = BM_Dequeue(free_b);
	if(BS[lba].alloc_block == NULL){
		printf("!!!full!!!\n");
		exit(2);
	}
	BS[lba].pba = BS[lba].alloc_block->PBA;
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

	for(int i = 0; i < valid_page_num + 1; i++){
		t_offset = d_sram[i].OOB_RAM.lpa % ppb_;
		SRAM_unload(d_sram, new_ppa + t_offset, i);
	}

	free(temp_set);
	free(d_sram);

	BM_InitializeBlock(BM, old_pba);
	BM_Enqueue(free_b, old_block);

	/* Trim block */
	__block.li->trim_block(old_ppa, false);
	return ;
}
