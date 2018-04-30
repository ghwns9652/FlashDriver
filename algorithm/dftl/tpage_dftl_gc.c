#ifdef UNIT_T
#include "dftl.h"

extern algorithm __demand;

extern C_TABLE *CMT; // Cached Mapping Table
extern D_OOB *demand_OOB; // Page level OOB
extern D_SRAM *d_sram; // SRAM for contain block data temporarily

extern int32_t DPA_status; // Data page allocation
extern int32_t TPA_status; // Translation page allocation
extern int32_t PBA_status; // Block allocation
extern int8_t needGC; // Indicates need of GC

char btype_check(int32_t PBA_status){
	int32_t PBA2PPA = PBA_status * _PPB;
	int invalid_page_num = 0;
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){
		if(!demand_OOB[i].valid_checker)
			invalid_page_num++;
	}
	if(invalid_page_num == _PPB)
		return 'N';

	for(int i = 0; i < CMTENT; i++){
		if(CMT[i].t_ppa != -1 && (CMT[i].t_ppa / _PPB) == PBA_status)
			return 'T';
	}
	return 'D';
}

int lpa_compare(const void *a, const void *b){
	uint32_t num1 = (uint32_t)(((D_SRAM*)a)->OOB_RAM.reverse_table);
	uint32_t num2 = (uint32_t)(((D_SRAM*)b)->OOB_RAM.reverse_table);
	if(num1 < num2) return -1;
	else if(num1 == num2) return 0;
	else return 1;
}

void dpage_GC(){
	int32_t lpa;
	int32_t vba;
	int32_t t_ppa;
	int32_t ppa;
	int32_t PBA2PPA;
	int32_t origin_ppa[_PPB];
	int valid_page_num;
	D_TABLE* p_table;
	value_set *temp_value_set;
	value_set *temp_value_set2;

	/* Load valid pages to SRAM */
	PBA2PPA = (PBA_status % _NOB) * _PPB; // PPA calculatioin
	valid_page_num = 0;
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){
		if(demand_OOB[i].valid_checker){ // Load valid pages
			SRAM_load(i, valid_page_num);
			origin_ppa[valid_page_num] = i;
			valid_page_num++;
		}
	}
	/* Trim data block */
	__demand.li->trim_block(PBA2PPA, false);

	/* Sort pages in SRAM */
	qsort(d_sram, _PPB, sizeof(D_SRAM), lpa_compare);	// Sort d_sram as lpa
	
	/* Mapping data manage */
	for(int i = 0; i < valid_page_num; i++){
		lpa = d_sram[i].OOB_RAM.reverse_table;
		p_table = CMT_check(lpa, &ppa);
		/* Cache update */
		if(p_table != NULL && ppa == origin_ppa[i]){ // Check cache bit
			p_table[P_IDX].ppa = PBA2PPA + i; // Cache ppa, flag update
			CMT[D_IDX].flag = 1;
		}
		/* Batch update */
		else{
			vba = INT32_MAX; // Initial state
			p_table = NULL;
			if(vba == D_IDX) // Share same vba with previous lpa
				p_table[P_IDX].ppa = PBA2PPA + i;
			else if(vba != INT32_MAX){ // Push t_ppa
				tp_alloc(&t_ppa);
				temp_value_set2 = inf_get_valueset(temp_value_set->value, DMAWRITE, PAGESIZE);
				__demand.li->push_data(t_ppa, PAGESIZE, temp_value_set2, 1, assign_pseudo_req());
				inf_free_valueset(temp_value_set2, DMAWRITE);
				demand_OOB[t_ppa] = (D_OOB){vba, 1}; // OOB management
				demand_OOB[CMT[vba].t_ppa].valid_checker = 0; // Invalidate previous tpage
				CMT[vba].t_ppa = t_ppa; // GTD update
				inf_free_valueset(temp_value_set, DMAREAD);
				p_table = NULL;
			}
			if(p_table == NULL && i != valid_page_num){ // Start to make new tpage
				vba = D_IDX; // vba calculation
				temp_value_set = inf_get_valueset(NULL, DMAREAD, PAGESIZE);
				__demand.li->pull_data(CMT[vba].t_ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Load tpage
				p_table = (D_TABLE*)temp_value_set->value;
				p_table[P_IDX].ppa = PBA2PPA + i; // Update tpage
			}
		}
	}
	for(int i = 0; i < valid_page_num; i++){
		SRAM_unload(PBA2PPA + i, i); // Unload dpages
	}
	DPA_status = PBA2PPA + valid_page_num;	// DPA_status update
}

int ppa_compare(const void *a, const void *b){
	uint32_t num1 = (uint32_t)(((D_TABLE*)a)->ppa);
	uint32_t num2 = (uint32_t)(((D_TABLE*)a)->ppa);
	if(num1 < num2) return -1;
	else if(num1 == num2) return 0;
	else return 1;
}

/* Please enhance the full merge algorithm !!! */
void tpage_GC(){
	int n = 0;
	int32_t head_ppa;
	D_TABLE* GTDcpy = (D_TABLE*)malloc(CMTENT * sizeof(D_TABLE));
	for(int i = 0; i < CMTENT; i++)
		GTDcpy[i].ppa = CMT[i].t_ppa;
	qsort(GTDcpy, CMTENT, sizeof(D_TABLE), ppa_compare);

	for(int i = 0; i < CMTENT; i++){
		if(GTDcpy[i].ppa == -1)
			break;
		else{ //
			SRAM_load(GTDcpy[i].ppa, n);
			if(n == 0)
				head_ppa = GTDcpy[i].ppa - GTDcpy[i].ppa % _PPB;
			if(n == _PPB){
				__demand.li->trim_block(head_ppa, false);
				for(int j = 0; j < _PPB; j++){
					CMT[d_sram[j].OOB_RAM.reverse_table].t_ppa = head_ppa + j;
					SRAM_unload(head_ppa + j, j);
				}
				n = -1;
			}
			n++;
		}
	}
	if(n > 0){
		__demand.li->trim_block(head_ppa, false);
		for(int j = 0; j < n; j++){
			CMT[d_sram[j].OOB_RAM.reverse_table].t_ppa = head_ppa + j;
			SRAM_unload(head_ppa + j, j);
		}
		TPA_status = head_ppa + n;
		n = 0;
	}
	free(GTDcpy);
}

void SRAM_load(int32_t ppa, int idx){
	value_set *temp_value_set;

	temp_value_set = inf_get_valueset(NULL, DMAREAD, PAGESIZE);
	__demand.li->pull_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Page load
	d_sram[idx].valueset_RAM = temp_value_set;
	d_sram[idx].OOB_RAM = demand_OOB[ppa];	// Page load
	demand_OOB[ppa] = (D_OOB){-1, 0};	// OOB init
}

void SRAM_unload(int32_t ppa, int idx){
	value_set *temp_value_set;

	temp_value_set = inf_get_valueset(d_sram[idx].valueset_RAM->value, DMAWRITE, PAGESIZE);
	__demand.li->push_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req());	// Page unlaod
	inf_free_valueset(temp_value_set, DMAWRITE);
	inf_free_valueset(d_sram[idx].valueset_RAM, DMAREAD);
	demand_OOB[ppa] = d_sram[idx].OOB_RAM;	// OOB unload
	d_sram[idx].valueset_RAM = NULL;	// SRAM init
	d_sram[idx].OOB_RAM = (D_OOB){-1, 0};
}

// Check the case when no page be GCed.
bool demand_GC(char btype){
	int32_t PBA2PPA = (PBA_status % _NOB) * _PPB;	// Change PBA to PPA
	char victim_btype;
	victim_btype = btype_check(PBA_status % _NOB);
	/* Case 0 : victim block type 'N' */
	if(victim_btype == 'N'){
		__demand.li->trim_block(PBA2PPA, false);
		if(btype == 'T')
			TPA_status = PBA2PPA;
		else
			DPA_status = PBA2PPA;
		return true;
	}
	else{
		/* Case 1 : victim block type 'T' */
		if(btype == 'T'){
			tpage_GC();
			return true;
		}
		/* Case 2 : victim block type 'D' */
		else{
			/* Check GCability */
			if(victim_btype != 'D')
				return false;
			for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){
				if(!demand_OOB[i].valid_checker)
					break;
				else if(i == PBA2PPA + _PPB - 1)
					return false;
			}
			/* Data block GC */
			dpage_GC();	// batch_update + t_page update
			return true;
		}
	}
	return false;
}
#endif
