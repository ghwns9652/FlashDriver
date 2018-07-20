#include "pftl.h"

int32_t pbase_garbage_collection(){
	int32_t old_block;
	int32_t new_block;
	uint8_t all;
	int valid_page_num;
	Block *victim;
	value_set **temp_set;
	SRAM *d_sram;

	all = 0;
	gc_count++;
	victim = BM_Heap_Get_Max(b_heap);
	if(victim->Invalid == p_p_b){ // if all invalid block
		all = 1;
	}
	else if(victim->Invalid == 0){
		printf("full!!!\n");
		exit(2);
	}
	//exchange block
	old_block = victim->PBA * p_p_b;
	new_block = reserved->PBA * p_p_b;
	reserved->hn_ptr = BM_Heap_Insert(b_heap, reserved);
	reserved = victim;
	if(all){ // if all page is invalid, then just trim and return
		algo_pbase.li->trim_block(old_block, false);
		BM_InvalidPPB_PBA(bm->barray, victim->PBA);
		return new_block;
	}
	valid_page_num = 0;
	gc_poll = 0;
	d_sram = (SRAM*)malloc(sizeof(SRAM) * p_p_b); //필요한 만큼만 할당하는 걸로 변경
	temp_set = (value_set**)malloc(sizeof(value_set*) * p_p_b);

	for(int i = 0; i < p_p_b; i++){
		d_sram[i].PTR_RAM = NULL;
		d_sram[i].OOB_RAM.lpa = -1;
	}

	/* read valid pages in block */
	for(int i = old_block; i < old_block + p_p_b; i++){
		if(BM_IsValidPage(bm, i)){ // read valid page
			temp_set[valid_page_num] = SRAM_load(d_sram, i, valid_page_num);
			valid_page_num++;
		}
	}

	while(gc_poll != valid_page_num) {} // polling for reading all mapping data
	
	for(int i = 0; i < valid_page_num; i++){ // copy data to memory and free dma valueset
		memcpy(d_sram[i].PTR_RAM, temp_set[i]->value, PAGESIZE);
		inf_free_valueset(temp_set[i], FS_MALLOC_R); //미리 value_set을 free시켜서 불필요한 value_set 낭비 줄임
	}

	gc_poll = 0;

	for(int i = 0; i < valid_page_num; i++){ // write page into new block
		SRAM_unload(d_sram, new_block + i, i);
	}

	while(gc_poll != valid_page_num) {} // polling for reading all mapping data

	BM_InvalidateBlock(bm, victim->PBA); // 알고리즘 질문

	free(temp_set);
	free(d_sram);

	/* Trim block */
	algo_pbase.li->trim_block(old_block, false);

	return new_block + valid_page_num;
}
