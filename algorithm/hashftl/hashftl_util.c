#include "hashftl.h"

algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req){
	algo_req *pseudo_my_req = (algo_req*)malloc(sizeof(algo_req));
	if(pseudo_my_req == NULL){
		printf("algo_req failed\n");
	}
	
	hash_params *params = (hash_params*)malloc(sizeof(hash_params));
	if(params == NULL){
		printf("hash_params faild\n");
	}

	pseudo_my_req->parents = req;
	pseudo_my_req->type = type;
	params->type = type;
	params->value = temp_v;
	switch(type){
		case DATA_R:
			pseudo_my_req->rapid = true;
			break;
		case DATA_W:
			pseudo_my_req->rapid = true;
			break;
		case GC_R:
			pseudo_my_req->rapid = false;
			break;
		case GC_W:
			pseudo_my_req->rapid = false;
			break;
	}
	pseudo_my_req->end_req = hash_end_req;
	pseudo_my_req->params = (void*)params;
	return pseudo_my_req;
}

value_set* SRAM_load(SRAM* sram, int32_t ppa, int idx){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	__hashftl.li->read(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req(GC_R, NULL, NULL)); // pull in gc is ALWAYS async
	sram[idx].PTR_RAM = (PTR)malloc(PAGESIZE);
	sram[idx].OOB_RAM.lpa = hash_OOB[ppa].lpa;
	return temp_value_set;
}

void SRAM_unload(SRAM* sram, int32_t ppa, int idx){
	value_set *temp_value_set;
	temp_value_set = inf_get_valueset((PTR)sram[idx].PTR_RAM, FS_MALLOC_W, PAGESIZE);
	__hashftl.li->write(ppa, PAGESIZE, temp_value_set, ASYNC, assign_pseudo_req(GC_W, temp_value_set, NULL));
	hash_OOB[ppa] = sram[idx].OOB_RAM;
	free(sram[idx].PTR_RAM);
}

/* extract pba from md5 */
int32_t get_pba_from_md5(uint64_t md5_result, int32_t hid){
	int32_t temp;
	uint64_t shifted;

	//shift result using hid;
	shifted = md5_result >> (hid&0x1ffff);

	temp = shifted % _g_nob;

	return temp;
}

/* check corresponding can be written for given lpa
if so, return ppa
if not return -1
*/
int32_t check_written( int32_t	pba, int32_t lpa, int32_t* cal_ppid)
{
	int32_t bit_fr_lpa, temp2, ppa;
	bit_fr_lpa = num_page_off - num_ppid;
	temp2 = bm->barray[pba].wr_off;

	// corresponding block is full;
	if(temp2 == _g_ppb){
		return -1;
	}

	if(temp2 > _g_ppb){
		printf("overflow\n");
	}

	if(bit_fr_lpa == 0){
		// ppid can represent all page offset
		ppa = (pba * _g_ppb) + temp2;
		*cal_ppid = temp2;
		return ppa;
	}
	else if(bit_fr_lpa > 0){
		//ppid cannot represent all page offset
		uint32_t lpn_v, i;
		i = pow(2,bit_fr_lpa);

		//get lsb from lpn
		lpn_v = lpa % i;

		//see lsb of lpn can represent appropriate page offset
		if((temp2 % i) == lpn_v){
			//if so, return page no and ppid
  			ppa = (pba * _g_ppb) + temp2;
			*cal_ppid = temp2 / i;
			return ppa;
		}
	}

	return -1;
}

/* get ppa from table using appropriate logic */
int32_t get_ppa_from_table(int32_t lpa){
	int32_t ppa;

	// this find logic is for convenience you should change it if you want real performance
	ppa = pri_table[lpa].ppa;

	/*if(pri_table[lpa].hid < hid_secondary){

	}
	else if(pri_table[lpa].hid == hid_secondary){

	}*/

	return ppa;
}

int32_t get_idx_for_secondary(int32_t lpa)
{
	int32_t i, temp;
	int k;

	i = pow(2, num_ppid);
	temp = max_secondary / i;

	for(k = temp * (pri_table[lpa].ppid); k < temp * ((pri_table[lpa].ppid) + 1); k++){
		if(sec_table[k].lpa == lpa){
			return k;
		}
	}

	printf("get idx for secondary: can't find entry for given lpa\n");
	exit(3);
	return max_secondary;
}


int32_t alloc_page(int32_t lpa, int32_t* cal_ppid, int32_t* hid){
	int32_t ppa, pba, cnt;
	uint32_t lpa_md5;
	//Block *block;
	static int32_t pba_secondary = 0;
	uint64_t* md5_res = NULL;
	size_t len = sizeof(lpa);

	if((md5_res = (uint64_t*)malloc(sizeof(uint64_t))) == NULL){
    	printf("alloc_page: md5_res allocate failed\n");
    	return 1;
  	}

  // get value from md5
 	lpa_md5 = lpa;

	md5(&lpa_md5, len, md5_res);

	while(*hid != hid_secondary){
    pba = get_pba_from_md5(*md5_res, *hid);

		if(pba != reserved_pba){
			if((ppa = check_written(pba, lpa, cal_ppid)) != -1){
				break;
			}
		}

		*hid = *hid + 1;
	}


	//if lpa cannot mapped to primary table, map to secondary table
	if(*hid == hid_secondary){
			//find appropriate block for free page
			cnt = 0;
			while(bm->barray[pba_secondary].wr_off == _g_ppb || pba_secondary == reserved_pba){
				pba_secondary++;

				if(pba_secondary >= _g_nob){
					pba_secondary = 0;
				}
				cnt++;

				if(cnt == _g_nob){
					printf("\n\ndo gc aloc\n");
					
					gc_val = 0;
					for(int i = 0; i < 8; i++){
						ppa = hash_garbage_collection();
					}
					pba_secondary = BM_PPA_TO_PBA(ppa);
					return ppa;
				}
			}

			ppa = pba_secondary * _g_ppb + bm->barray[pba_secondary].wr_off;
	}
	free(md5_res);
	return ppa;
}

int32_t map_for_gc(int32_t lpa, int32_t ppa){
	int32_t idx_secondary,temp;
	Block *block;


	if(pri_table[lpa].state == H_VALID){
		if(pri_table[lpa].hid == hid_secondary){
			idx_secondary =	get_idx_for_secondary(lpa);

			sec_table[idx_secondary].state = CLEAN;
			sec_table[idx_secondary].ppa = -1;
			sec_table[idx_secondary].lpa = -1;
			num_secondary--;
		}
	}

	idx_secondary = max_secondary;

	//find free entry at secondary table
	for(int k = 0; k < max_secondary; k++){
		if(sec_table[k].state == CLEAN){
			idx_secondary = k;
			sec_table[k].state = H_VALID;
			break;
		}
	}

	if(idx_secondary == max_secondary){
		printf ("\nmap for gc: secondary table is fulled\n");
		printf("max secondary table: %d\n", max_secondary);
		printf("num secondary table: %d\n", num_secondary);
		printf("Requested writes: %d\n", num_write);
		printf("Copyed writes: %d\n", num_copy);
		printf("WAF: %d\n", ((num_write+num_copy)*100/num_write));
	
		exit(3);
		return 1;
	}

	pri_table[lpa].hid = hid_secondary;
	pri_table[lpa].ppa = ppa;
	pri_table[lpa].state = H_VALID;
	block = &bm->barray[BM_PPA_TO_PBA(ppa)];
	if(block->wr_off == 0){
		block->hn_ptr = BM_Heap_Insert(b_heap, block);
	}
	block->wr_off++;


  // calculate ppid for secondary table
	temp = pow(2, num_ppid);
	temp = temp*idx_secondary;
	temp = temp/max_secondary;
	pri_table[lpa].ppid = temp;

  //set lpa, ppa, status
  	sec_table[idx_secondary].ppa = ppa;
 	sec_table[idx_secondary].lpa = lpa;
  	sec_table[idx_secondary].state = H_VALID;
 	hash_OOB[ppa].lpa = lpa;
	BM_ValidatePage(bm, ppa);
	num_secondary++;

  	return 0;
}

/* 
	given lpa is remapped to new ppa and that information is
	stored at primary table
	clean corresponding secondary table and invalidate page
	change lpa's mapping to primary table
*/
int32_t map_for_remap(
	int32_t lpa, 
	int32_t ppa, 
	int32_t hid,  
	int32_t* cal_ppid,
	int32_t secondary_idx){

	Block *block;

	if(lpa != sec_table[secondary_idx].lpa){
		printf("\nmap for remap: lap mapping is wrong\n");
		exit(3);
	}

	if(sec_table[secondary_idx].state != H_VALID){
		printf("\nmap for remap: state is not valid\n");
		exit(3);
	}

	if(pri_table[lpa].hid != hid_secondary){
		printf("\nmap for remap: original hid is not secondary\n");
		exit(3);
	}

	BM_InvalidatePage(bm, sec_table[secondary_idx].ppa);
	sec_table[secondary_idx].state = CLEAN;
	sec_table[secondary_idx].ppa = -1;
	sec_table[secondary_idx].lpa = -1;
	num_secondary--;

	pri_table[lpa].ppa = ppa;
	pri_table[lpa].ppid = *cal_ppid;
	pri_table[lpa].hid = hid;
	pri_table[lpa].state = H_VALID;
	hash_OOB[ppa].lpa = lpa;
	BM_ValidatePage(bm, ppa);

	block = &bm->barray[BM_PPA_TO_PBA(ppa)];
	if(block->wr_off == 0){
		block->hn_ptr = BM_Heap_Insert(b_heap, block);
	}
	block->wr_off++;
	if(block->wr_off > _g_ppb){
		printf("\nmap for remap: overflow page offset\n");
		exit(3);
	}
	
	return 0;
}


int32_t remap_sec_to_pri(int32_t pba, int32_t* cal_ppid){
	int32_t lpa, ppa, prev_ppa, hid, pba_t;
	uint32_t lpa_md5;
	int count = 0;

	size_t len = sizeof(lpa);
	

	for(int i =0; i < max_secondary; i++){
		if(sec_table[i].state == H_VALID){
			uint64_t md5_res;

			lpa = sec_table[i].lpa;
			lpa_md5 = lpa;
			
			md5_res = md5_u(&lpa_md5, len);
			prev_ppa = sec_table[i].ppa;
			hid = 1;
			while(hid != hid_secondary){

				pba_t = get_pba_from_md5(md5_res, hid);
				if(pba_t != reserved_pba){
					if((ppa = check_written(pba_t, lpa, cal_ppid)) != -1){
						value_set *temp_set = NULL;
						value_set *temp_value_set = NULL;
						gc_load = 0;						
						
						temp_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);	
						__hashftl.li->read(prev_ppa, PAGESIZE, temp_set, 1,assign_pseudo_req(GC_R, NULL, NULL));
						
						
						while(gc_load == 0){}

						temp_value_set = inf_get_valueset((PTR)temp_set->value, FS_MALLOC_W, PAGESIZE);

						map_for_remap(lpa, ppa, hid, cal_ppid, i);
						__hashftl.li->write(ppa, PAGESIZE,temp_value_set,ASYNC,assign_pseudo_req(GC_W, temp_value_set, NULL));
						
						hash_OOB[prev_ppa].lpa = -1;
						free(temp_set);
						count++;

						//check the number of page that are moved during remapping
						re_page_count++;
						re_number++;
						num_copy++;
						break;
					}
				}
				hid = hid + 1;
			}
		}
	}
	printf("remapped count: %d\n\n", count);

	return 0;
}
