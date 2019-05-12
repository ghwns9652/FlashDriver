#include "hashftl.h"


int32_t hash_garbage_collection(){
	/* garbage collection for hashftl.
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
	printf("gc_count: %d\n", gc_count);
	printf("secondary entry: %d\n", num_secondary);

  	victim = BM_Heap_Get_Max(b_heap);
    if(victim->Invalid == _g_ppb){ // if all invalid block
        all = 1;
    }
    else if(victim->Invalid == 0){
        printf("block info\n");
		for(int i = 0; i < _g_nob; i++){
			printf("block number: %d\n", i);
			printf("invalid number: %d\n", bm->barray[i].Invalid);
			printf("write offset: %d\n", bm->barray[i].wr_off);
		}
		printf("\n!!!full!!!\n");
        exit(2);
    }


    //exchange block
    victim->Invalid = 0;
    victim->wr_off = 0;
	old_block = victim->PBA * _g_ppb;
    new_block = reserved->PBA * _g_ppb;
//  	reserved->hn_ptr = BM_Heap_Insert(b_heap, reserved);
  	reserved = victim;
	reserved_pba = victim->PBA;


  	if(all){ // if all page is invalid, then just trim and return
	    puts("GC() - all");
  		__hashftl.li->trim_block(old_block, false);
      	BM_InitializeBlock(bm, victim->PBA);
  		return new_block;
  	}
    printf("GC()");

    valid_page_num = 0;
    gc_load = 0;
    d_sram = (SRAM*)malloc(sizeof(SRAM) * _g_ppb);
    temp_set = (value_set**)malloc(sizeof(value_set*) * _g_ppb);


  	for(int i=0;i<_g_ppb;i++){
  		d_sram[i].PTR_RAM = NULL;
  		d_sram[i].OOB_RAM.lpa = -1;
  	}


  	/* read valid pages in block */
  	for(int i=old_block;i<old_block+_g_ppb;i++){
  		if(BM_IsValidPage(bm, i)){ // read valid page
  			temp_set[valid_page_num] = SRAM_load(d_sram, i, valid_page_num);
  			valid_page_num++;

			if(pri_table[hash_OOB[i].lpa].hid < hid_secondary && pri_table[hash_OOB[i].lpa].hid != 0){
  				gc_pri++;
			}
		}
  	}

    BM_InitializeBlock(bm, victim->PBA);

  	while(gc_load != valid_page_num) {} // polling for reading all mapping data

    for(int i = 0; i < valid_page_num; i++){ // copy data to memory and free dma valueset
      memcpy(d_sram[i].PTR_RAM, temp_set[i]->value, PAGESIZE);
          inf_free_valueset(temp_set[i], FS_MALLOC_R); //미리 value_set을 free시켜서 불필요한 value_set 낭비 줄임
	  //if(temp_set[i]->value != NULL) free(temp_set[i]->value);
	  // free(temp_set[i]);
	}


    for(int i = 0; i < valid_page_num; i++){ // write page into new block
        map_for_gc(d_sram[i].OOB_RAM.lpa, new_block + i);
        SRAM_unload(d_sram, new_block + i, i);
    }


    free(temp_set);
    free(d_sram);

        /* Trim block */
    __hashftl.li->trim_block(old_block, false);

  	printf(" - %d\n", valid_page_num);
	gc_val = gc_val + valid_page_num;
	num_copy = num_copy + valid_page_num;

    return new_block + valid_page_num;
}
