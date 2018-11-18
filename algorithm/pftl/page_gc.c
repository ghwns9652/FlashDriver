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
	if(victim->Invalid == _g_ppb * ALGO_SEGNUM){ // every page is invalid.
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
    for(int i=0; i<_g_ppb;i++)
        d_sram[i].OOB_RAM = (P_OOB*)malloc(sizeof(P_OOB)*ALGO_SEGNUM);
	temp_set = (value_set**)malloc(sizeof(value_set*) * _g_ppb);

	for(int i=0;i<_g_ppb;i++){
		d_sram[i].PTR_RAM = NULL;
        for (int j=0;j<ALGO_SEGNUM;j++)
		    d_sram[i].OOB_RAM[j].lpa = -1;
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
	for(int i=0;i<valid_page_num;i++){// copy datas into memory.
        memcpy(d_sram[i].PTR_RAM, temp_set[i]->value, PAGESIZE);
        inf_free_valueset(temp_set[i],FS_MALLOC_R);
    }
    
    /*!!data loading is done!!*/
    printf("data loading done.\n");

#ifdef LARGEPAGE
    SRAM gcbuf_SRAM;
    gcbuf_SRAM.PTR_RAM = (PTR)malloc(PAGESIZE);
    gcbuf_SRAM.OOB_RAM = (P_OOB*)malloc(sizeof(P_OOB)*ALGO_SEGNUM);
	int8_t gcbuf_full = 0;
	int8_t gcbuf_slot = 0;
	int32_t offset = 0;
    //printf("data unloading init done\n");
	for(int i=0;i<_g_ppb * ALGO_SEGNUM;i++){//search through all SEGMENTS.
		if(BM_IsValidPage(BM,old_block + i)){
            int dest = gcbuf_slot*ALGO_SEGSIZE;
            int src = i/ALGO_SEGNUM;
            int src_offset = i % ALGO_SEGNUM;
			memcpy(&(gcbuf_SRAM.PTR_RAM[dest]),
                   &(d_sram[src].PTR_RAM[src_offset*ALGO_SEGSIZE]),
                   ALGO_SEGSIZE);

			BM_ValidatePage(BM,new_block+i);
			gcbuf_slot++;
			
			if(gcbuf_slot == ALGO_SEGNUM){//gcbuf full!!
				SRAM_unload(&gcbuf_SRAM,new_block+offset,0);
                for(int i=0;i<ALGO_SEGNUM;i++)
                    BM_ValidatePage(BM,new_block+offset*i);
                offset++;
				gcbuf_slot = 0;
                //gcbuf_sram reset.
				memset(gcbuf_SRAM.PTR_RAM,0,PAGESIZE);
                for(int i=0;i<ALGO_SEGNUM;i++)
                    gcbuf_SRAM.OOB_RAM[i].lpa = -1;
                //!gcbuf_sram reset
			}//!gcbuf full
		}//!valid page logic
        if(gcbuf_slot != 0){//some leftovers.
            SRAM_unload(&gcbuf_SRAM,new_block+offset,0);
            for(int i=1;i<gcbuf_slot;i++)
                BM_ValidatePage(BM,new_block+offset*i);
            offset++;
        }
        //free(gcbuf_SRAM.OOB_RAM);
        //free(gcbuf_SRAM.PTR_RAM);
	}

#else
	for(int i=0;i<valid_page_num;i++){ // write page into new block
		SRAM_unload(d_sram, new_block + i, i);
        free(d_sram[i].OOB_RAM);
        free(d_sram[i].PTR_RAM);
		BM_ValidatePage(BM,new_block+i);
	}
#endif

	free(temp_set);
	free(d_sram);

	/* Trim block */
	algo_pbase.li->trim_block(old_block, false);

#ifdef LARGEPAGE
    return new_block + offset;
#else
	return new_block + valid_page_num;
#endif
}
