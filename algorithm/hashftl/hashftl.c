#include "hashftl.h"
#include "../../bench/bench.h"


/* HASHFTL methods */
algorithm __hashftl = {
	.create  = hash_create,
	.destroy = hash_destroy,
	.read    = hash_get,
	.write   = hash_set,
	.remove  = hash_remove
};

//heap globals.
b_queue *free_b;
Heap *b_heap;

PRIMARY_TABLE *pri_table;     // primary table
SECONDARY_TABLE *sec_table;   //secondary table
H_OOB *hash_OOB;   //OOB area.

//blockmanager globals.
BM_T *bm;
Block *reserved;    //reserved.
int32_t reserved_pba;


//global for macro.
int32_t _g_nop;				// number of page
int32_t _g_nob;				// number of block
int32_t _g_ppb;				// page per block

int32_t max_secondary;        // secondary table size
int32_t num_secondary;        // allocated secondary table
int32_t num_hid;              // number of hid bits
int32_t num_ppid;             // number of ppid bits
int32_t num_page_off;          // number of page offset bits
int32_t hid_secondary;        // hid that indicate mapping is at secondary table
int32_t loop_gc;			// number of gc to immitate chip level parallelism

int32_t gc_load;
int32_t gc_count;			// number of total gc
int32_t re_gc_count;		// number of gc due to remap
int32_t re_page_count;		// number of moved page during remap

// info during remap
int32_t gc_val;
int32_t re_number;
int32_t gc_pri;

int32_t num_write;
int32_t num_copy;



static void print_algo_log() {
	printf("\n");
	printf(" |---------- algorithm_log : Hash FTL\n");

	printf(" | Total Blocks(Segments): %d\n", _g_nob);
	printf(" | Total Pages:            %d\n", _g_nop);
	printf(" |  -Page per Block:       %d\n\n", _g_ppb);

	printf(" | Hash ID bits :          %d\n", num_hid);
	printf(" | ppid bits:              %d\n", num_ppid);
	printf(" | page offset bits:       %d\n", num_page_off);
	printf(" | Total secondary entries:    %d\n", max_secondary);
}

uint32_t hash_create(lower_info *li, algorithm *algo){
	int temp, bit_cnt;
	FILE *fp_out;

	printf("Start create hash FTL\n");
	/* Initialize pre-defined values by using macro */
	//    _g_nop = _NOP;
	//    _g_nob = _NOS;
	//    _g_ppb = _PPS;

	_g_nop = _NOP;
	_g_nob = _NOB;
	_g_ppb = _PPB;

	gc_count = 0;
	re_gc_count = 0;
	re_page_count = 0;
	loop_gc = 8;

	// Secondary table size
	max_secondary = _g_nop / 8;

	temp = _g_ppb;
	bit_cnt = 0;
	while(temp != 0){
		temp = temp/2;
		bit_cnt++;
	}

	num_secondary = 0;
	num_hid = 3;
	num_page_off = bit_cnt - 1;
	num_ppid = num_page_off - 1;
	hid_secondary = pow(2, num_hid) - 1;

	num_write = 0;
	num_copy = 0;

	// Map lower info
	algo->li = li;

	//print information
	print_algo_log();

	bm = BM_Init(_g_nob, _g_ppb, 1, 1);

	reserved = &bm->barray[0];
	reserved_pba = 0;
	BM_Queue_Init(&free_b);
	for(int i=1;i<_g_nob;i++){
		BM_Enqueue(free_b, &bm->barray[i]);
	}
	b_heap = BM_Heap_Init(_g_nob - 1);//total size == NOB - 1.

	bm->harray[0] = b_heap;
	bm->qarray[0] = free_b;


	pri_table = (PRIMARY_TABLE*)malloc(sizeof(PRIMARY_TABLE) * _g_nop);
	hash_OOB = (H_OOB*)malloc(sizeof(H_OOB) * _g_nop);
	sec_table = (SECONDARY_TABLE*)malloc(sizeof(SECONDARY_TABLE) * max_secondary);

	// initialize primary table & oob
	for(int i=0;i<_g_nop;i++){
		pri_table[i].hid = 0;
		pri_table[i].ppid = -1;
		pri_table[i].ppa = -1;

		pri_table[i].state = CLEAN;

		hash_OOB[i].lpa = -1;
	}

	// initialize primary table & oob
	for(int i=0;i<max_secondary;i++){
		sec_table[i].ppa = -1;
		sec_table[i].lpa = -1;


		sec_table[i].state = CLEAN;

	}

	fp_out = fopen("Remap info.txt", "w");
	fprintf(fp_out, "The number of HID bits: %d\t\r\n", num_hid);
	fprintf(fp_out, "The number of PPID bits: %d\t\r\n", num_ppid);
	fprintf(fp_out, "The number of Page offset bits: %d\t\r\n", num_page_off);
	fprintf(fp_out, "Total secondary entries: %d\t\r\n", max_secondary);
	fprintf(fp_out, "#\t# of valid\t# of primary\t# of remapped\r\n");
	fclose(fp_out);

	return 0;
}


void *hash_end_req(algo_req* input){
	/*
	 * end req differs according to type.
	 * frees params and input.
	 * in some case, frees inf_req.
	 */
	hash_params *params = (hash_params*)input->params;
	value_set *temp_set = params->value;
	request *res = input->parents;

	switch(params->type){
		case DATA_R:
			if(res){
				res->end_req(res);
			}
			break;
		case DATA_W:
			if(res){
				res->end_req(res);
			}
			break;
		case GC_R:
			gc_load++;
			//inf_free_valueset(temp_set, FS_MALLOC_R);
			//free(temp_set);
			break;
		case GC_W:
			if(temp_set == NULL){
				printf("\nhash_end_req: temp_set is NULL\n");
			}
			inf_free_valueset(temp_set, FS_MALLOC_W);
			//free(temp_set);
			break;
	}
	free(params);
	free(input);
	return NULL;
}

void hash_destroy(lower_info *li, algorithm *algo){
	FILE *fp_out;
	FILE *sec_out;
	FILE *bl_out;

	puts("");


	/* Print information */
	printf("Hash FTL summary start---------------------------------\n");
	printf("# of gc: %d\n", gc_count/8);
	printf("# of gc due to remap: %d\n", re_gc_count);
	printf("# of moved page due to remap: %d\n\n", re_page_count);


	printf("Total secondary entries: %d\n", max_secondary);
	printf("Used secondary entries: %d\n\n", num_secondary);
	printf("Requested writes: %d\n", num_write);
	printf("Copyed writes: %d\n", num_copy);
	printf("WAF: %d\n", ((num_write+num_copy)*100/num_write));
	printf("Hash FTL summary end----------------------------------\n");

	//printf("WAF: %.2f\n\n", (float)(data_r+dirty_evict_on_write)/data_r);

	// print FTL info to .txt file
	fp_out = fopen("ftl_info.txt", "w");
	fprintf(fp_out, "The number of HID bits: %d\t\r\n", num_hid);
	fprintf(fp_out, "The number of PPID bits: %d\t\r\n", num_ppid);
	fprintf(fp_out, "The number of Page offset bits: %d\t\r\n", num_page_off);
	fprintf(fp_out, "Total secondary entries: %d\t\r\n", max_secondary);
	fprintf(fp_out, "Used secondary entries: %d\t\r\n", num_secondary);
	fprintf(fp_out, "LPA\tPPA\tHID\tPPID\n");

	for(int i = 0 ; i < _g_nop ; i++){
		fprintf(fp_out, "%d\t", i);
		fprintf(fp_out, "%d\t", pri_table[i].ppa);
		fprintf(fp_out, "%d\t", pri_table[i].hid);
		fprintf(fp_out, "%d\t\r\n", pri_table[i].ppid);
		//fprintf(fp_out, "%d\n", hash_OOB[i].lpa);
	}
	fclose(fp_out);

	// secondary table info print
	sec_out = fopen("ftl_secondary_info.txt", "w");
	fprintf(sec_out, "Total secondary entries: %d\t\r\n", max_secondary);
	fprintf(sec_out, "Used secondary entries: %d\t\r\n", num_secondary);
	fprintf(sec_out, "IDX\tLPA\tPPA\tSTATE\n");

	for(int i = 0 ; i < max_secondary ; i++){
		fprintf(sec_out, "%d\t", i);
		fprintf(sec_out, "%d\t", sec_table[i].lpa);
		fprintf(sec_out, "%d\t", sec_table[i].ppa);
		fprintf(sec_out, "%d\t\r\n", sec_table[i].state);
	}
	fclose(sec_out);


	// print blk info to .txt file
	bl_out = fopen("blk_info.txt", "w");
	fprintf(bl_out, "Total block: %d\r\n", _g_nob);
	fprintf(bl_out, "Pages per block: %d\r\n", _g_ppb);
	fprintf(bl_out, "PBA\tWrite Offset\tInvalid\r\n");

	for(int i = 0 ; i < _g_nob; i++){
		fprintf(bl_out, "%d\t", i);
		fprintf(bl_out, "%d\t\t", bm->barray[i].wr_off);
		fprintf(bl_out, "%d\t\r\n", bm->barray[i].Invalid);
	}
	fclose(bl_out);
	/* Clear modules */
	BM_Free(bm);


	/* Clear tables */
	free(pri_table);
	free(sec_table);
	free(hash_OOB);
}

uint32_t hash_get(request* const req){
	/*
	 * gives pull request to lower level.
	 * reads mapping data.
	 * !!if not mapped, does not pull!!
	 */
	int32_t lpa;
	int32_t ppa;

	bench_algo_start(req);
	lpa = req->key;
	ppa = get_ppa_from_table(lpa);
	if(ppa == -1){
		bench_algo_end(req);
		req->type = FS_NOTFOUND_T;
		req->end_req(req);
		return 1;
	}
	bench_algo_end(req);
	__hashftl.li->read(ppa, PAGESIZE, req->value, ASYNC, assign_pseudo_req(DATA_R, NULL, req));
	return 0;
}

uint32_t hash_set(request* const req){
	int32_t lpa, temp, temp_pba;
	int32_t ppa;
	int32_t idx_secondary;
	int32_t* hid = NULL;
	int32_t* cal_ppid = NULL;
	Block *block;
	algo_req *my_req;
	int cnt;
	FILE *fp_out;

	if((hid = (int32_t*)malloc(sizeof(int32_t))) == NULL){
		printf("hash_set: hid allocate failed\n");
		return 1;
	};
	if((cal_ppid = (int32_t*)malloc(sizeof(int32_t))) == NULL){
		printf("hash_set: cal_ppid allocate failed\n");
		return 1;
	};

	//bench_algo_start(req);
	lpa = req->key;
	if(lpa > RANGE + 1){
		printf("hash set: range error\n");
		exit(3);
	}
	*hid = 1;
	ppa = alloc_page(lpa, cal_ppid, hid);

	//bench_algo_end(req);


	my_req = assign_pseudo_req(DATA_W, NULL, req);

	__hashftl.li->write(ppa, PAGESIZE, req->value, ASYNC, my_req);

	num_write++;

	if(pri_table[lpa].hid != 0){//already mapped case.(update)
		BM_InvalidatePage(bm ,get_ppa_from_table(lpa));
		if(pri_table[lpa].hid == hid_secondary){
			idx_secondary = get_idx_for_secondary(lpa);

			sec_table[idx_secondary].state = CLEAN;
			sec_table[idx_secondary].ppa = -1;
			sec_table[idx_secondary].lpa = -1;
			num_secondary--;
		}
	}

	// put information to table and validate page
	pri_table[lpa].hid = *hid;
	pri_table[lpa].state = H_VALID;
	block = &bm->barray[BM_PPA_TO_PBA(ppa)];
	if(block->wr_off == 0){
		//		printf("start\n");
		//		printf("heap pba : %d\n", BM_PPA_TO_PBA(ppa));
		block->hn_ptr = BM_Heap_Insert(b_heap, block);	
		//		printf("end\n");
	}
	block->wr_off++;

	/* if you want to check real hash ftl performance,
	   you should change find ppa logic (hashftl_util.c -> get_ppa_from_table)
	   for now, primary table contain ppa for convenience*/
	pri_table[lpa].ppa = ppa;

	if(*hid < hid_secondary){          //if hid is primary
		pri_table[lpa].ppid = *cal_ppid;
	}
	else if(*hid == hid_secondary){    //if hid is secondary
		idx_secondary = max_secondary;

		//find free entry at secondary table
		for(int k = 0; k < max_secondary; k++){
			if(sec_table[k].state == CLEAN){
				idx_secondary = k;
				sec_table[k].state = H_VALID;
				break;
			}
		}

		//there are no free entries
		if(idx_secondary == max_secondary){
			printf ("\nhash set: secondary table is fulled\n");
			printf("max secondary table: %d\n", max_secondary);
			printf("num secondary table: %d\n", num_secondary);
			printf("Requested writes: %d\n", num_write);
			printf("Copyed writes: %d\n", num_copy);
			printf("WAF: %d\n", ((num_write+num_copy)*100/num_write));

			free(hid);
			free(cal_ppid);
			exit(3);
			return 1;
		}

		// calculate ppid for secondary table
		temp = pow(2, num_ppid);
		temp = temp * idx_secondary;
		temp = temp / max_secondary;
		pri_table[lpa].ppid = temp;

		//set lpa, ppa, status
		sec_table[idx_secondary].ppa = ppa;
		sec_table[idx_secondary].lpa = lpa;
		sec_table[idx_secondary].state = H_VALID;
		num_secondary++;

		if((num_secondary * 100 / max_secondary) >= 70){
			printf("\ndo gc sec\n");
			cnt = 0;

			fp_out = fopen("Remap info.txt", "a");
			while((num_secondary * 100 / max_secondary) >= 50){
				//check the number of gc due to remap
				re_gc_count++;

				gc_val = 0;
				re_number = 0;
				gc_pri = 0;
				for(int i = 0; i < loop_gc ; i++){
					temp_pba = hash_garbage_collection();
					temp_pba = BM_PPA_TO_PBA(temp_pba);
					remap_sec_to_pri(temp_pba, cal_ppid);
				}
				cnt++;

				fprintf(fp_out, "%d\t%d\t\t%d\t\t%d\r\n", re_gc_count, gc_val, gc_pri, re_number);

			}
			fprintf(fp_out, "----------------------------Remap end\r\n");
			fclose(fp_out);
		}
	}

	BM_ValidatePage(bm, ppa);
	hash_OOB[ppa].lpa = lpa;
	free(hid);
	free(cal_ppid);


	return 0;
}


uint32_t hash_remove(request* const req){

	int32_t lpa;
	int32_t ppa, idx_secondary;
	//hash_params * params;

	bench_algo_start(req);
	lpa = req->key;
	if (lpa > RANGE + 1) {
		printf("range error %d\n",lpa);
		exit(3);
	}

	ppa = pri_table[lpa].ppa;

	if(pri_table[lpa].hid == hid_secondary){
		idx_secondary = get_idx_for_secondary(lpa);
		sec_table[idx_secondary].lpa = -1;
		sec_table[idx_secondary].ppa = -1;
		sec_table[idx_secondary].state = CLEAN;
	}


	pri_table[lpa].ppa = -1; //reset to default.
	hash_OOB[lpa].lpa = -1; //reset reverse_table to default.
	pri_table[lpa].hid = 0;
	pri_table[lpa].state = CLEAN;

	BM_InvalidatePage(bm, ppa);
	bench_algo_end(req);
	return 0;
}
