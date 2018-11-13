#include "page.h"

int32_t pbase_garbage_collection(){
	/* garbage collection for pftl.
	 * find victim, exchange with op area.
	 * SRAM_load -> SRAM_unload -> trim.
	 * need to update mapping data as well.
	 */
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
	if(victim->Invalid == _g_ppb){ // every page is invalid.
		all = 1;
	}
	else if(victim->Invalid == 0){// ssd all valid.
		printf("full!!!\n");
		exit(2);
	}
	//exchange block(!! trim does not know about large page condition.!!)

	old_block = victim->PBA * _g_ppb;
	new_block = reserved->PBA * _g_ppb;
	reserved->hn_ptr = BM_Heap_Insert(b_heap, reserved);//current rsv goes into heap.
	reserved = victim;//rsv ptr points to victim.
	if(all){ // if all page is invalid, then just trim and return
		algo_pbase.li->trim_block(old_block, false);
		BM_InvalidZero_PBA(BM, victim->PBA);
		return new_block * ALGO_SEGNUM;
	}
	valid_page_num = 0;
	gc_load = 0;
	d_sram = (SRAM*)malloc(sizeof(SRAM) * _g_ppb);
	temp_set = (value_set**)malloc(sizeof(value_set*) * _g_ppb);

	for(int i=0;i<_g_ppb;i++){
		d_sram[i].PTR_RAM = NULL;
		d_sram[i].OOB_RAM.lpa = -1;
	}

#ifdef LARGEPAGE
	/*large-page load*/
	for (int i=old_block; i<old_block + _g_ppb; i++){
		temp_set[valid_page_num] = SRAM_load(d_sram, i, valid_page_num);
		valid_page_num++;
	}

#else
	/*non large-page load*/
	/* read valid pages in block */
	for(int i=old_block;i<old_block+_g_ppb;i++){
		if(BM_IsValidPage(BM,i)){ // read valid page
			temp_set[valid_page_num] = SRAM_load(d_sram, i, valid_page_num);
			valid_page_num++;
		}
	}
	/*!non large-page load*/
#endif

	BM_InitializeBlock(BM, victim->PBA);
	
	while(gc_load != valid_page_num){} // polling for reading all mapping data
	
#ifdef LARGEPAGE
	char gc_buffer[PAGESIZE];
	int8_t gcbuf_full = 0;
	int8_t gcbuf_slot = 0;
	int32_t offset = 0;
	for(int i=0;i<_g_ppb * ALGO_SEGNUM;i++){//search through all SEGMENTS.
		if(BM_IsValidPage(BM,old_block + i)){
			memcpy(dest,src,SEGSIZE);
			BM_ValidatePage(BM,new_block+i);
			gcbuf_slot++;
			
			if(gcbuf_slot == ALGO_SEGNUM){//gcbuf full!!
				SRAM_unload(gc_buffer,new_block+offset,0);
				gcbuf_slot = 0;
				memset(gc_buffer,0,PAGESIZE);
			}
		}
		if((i % ALGO_SEGNUM) == ALGO_SEGNUM-1){
			int a = i / ALGO_SEGNUM;
			inf_free_valueset(temp_set[a],FS_MALLOC_R;
		}
	}

#else
	for(int i=0;i<valid_page_num;i++){ // copy data to memory and free dma valueset
		memcpy(d_sram[i].PTR_RAM, temp_set[i]->value, PAGESIZE);
		inf_free_valueset(temp_set[i], FS_MALLOC_R); //free value_set to reduce expanse of value_set.
	}

	for(int i=0;i<valid_page_num;i++){ // write page into new block
		SRAM_unload(d_sram, new_block + i, i);
		BM_ValidatePage(BM,new_block+i);
	}

#endif

	free(temp_set);
	free(d_sram);

	/* Trim block */
	algo_pbase.li->trim_block(old_block, false);

	return new_block + valid_page_num;
}
