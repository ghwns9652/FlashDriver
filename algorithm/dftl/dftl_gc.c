#ifdef UNIT_D
#include "dftl.h"

extern algorithm __demand;

extern C_TABLE *CMT; // Cached Mapping Table
extern D_TABLE *GTD; // Global Translation Directory
extern D_OOB *demand_OOB; // Page level OOB
extern D_SRAM *d_sram; // SRAM for contain block data temporarily

/* Please erase global variable */
extern int32_t DPA_status; // Data page allocation
extern int32_t TPA_status; // Translation page allocation
extern int32_t PBA_status; // Block allocation
extern int8_t needGC; // Indicates need of GC

/* btype_check
 * Determine a type of block
 * If there are only invalid pages, block type is 'N'
 * Else if the block is mapped on GTD, block type is 'T'
 * Else block type is 'D'
 */
char btype_check(){
	int32_t PBA2PPA = (PBA_status % _NOB) * _PPB; // Convert PBA to PPA
	/*류현진의 명언을 잊지 말라, remember ryu
	  add block structure
	 */
	int invalid_page_num = 0;
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){ // Count the number of invalid pages in block
		if(!demand_OOB[i].valid_checker)
			invalid_page_num++;
	}
	if(invalid_page_num == _PPB)
		return 'N'; // No valid page in this block

	for(int i = 0; i < GTDENT; i++){
		if(GTD[i].ppa != -1 && (GTD[i].ppa / _PPB) == PBA_status) // GTD search
			return 'T'; // Translation block
	}
	return 'D'; // Data block
}

/* lpa_compare
 * Used to sort pages as an order of lpa
 * Used in dpage_GC to sort pages that are in a GC victim block
 */
int lpa_compare(const void *a, const void *b){
	uint32_t num1 = (uint32_t)(((D_SRAM*)a)->OOB_RAM.reverse_table);
	uint32_t num2 = (uint32_t)(((D_SRAM*)b)->OOB_RAM.reverse_table);
	if(num1 < num2) return -1;
	else if(num1 == num2) return 0;
	else return 1;
}

/* dpage_GC
 * GC only one data block that is indicated by PBA_status
 * Load valid pages in a GC victim block to SRAM
 * Sort them by an order of lpa
 * If mapping is on cache, update mapping data in cache
 * If mapping is on flash, do batch update mapping data in translation page
 * 'Batch update' updates mapping datas in one translation page at the same time
 * After managing mapping data, write data pages to victim block
 */
void dpage_GC(){
	int32_t lpa;
	int32_t vba; // virtual block address (unit of translation page)
	int32_t t_ppa;
	int32_t ppa;
	int32_t PBA2PPA;
	int32_t origin_ppa[_PPB];
	int valid_page_num;
	int CMT_i; // CMT index
	D_TABLE* p_table; // mapping table in translation page
	value_set *temp_value_set;
	value_set *temp_value_set2;

	/* Load valid pages to SRAM */
	PBA2PPA = (PBA_status % _NOB) * _PPB; // Convert PBA to PPA
	valid_page_num = 0;
	for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){ // Load valid pages in block
		if(demand_OOB[i].valid_checker){ 
			SRAM_load(i, valid_page_num);
			origin_ppa[valid_page_num] = i;
			valid_page_num++;
		}
	}
	/* Trim data block */
	__demand.li->trim_block(PBA2PPA, false);

	/* Sort pages in SRAM */
	qsort(d_sram, _PPB, sizeof(D_SRAM), lpa_compare); // Sort valid pages by lpa order
	
	/* Manage mapping data and write tpages */
	for(int i = 0; i <= valid_page_num; i++){
		lpa = d_sram[i].OOB_RAM.reverse_table; // Get lpa of a page
		CMT_i = CMT_check(lpa, &ppa); // Search cache
		/* If valid mapping in cache, cache update */
		if(CMT_i != -1 && ppa == origin_ppa[i]){ // Check valid mapping location
			CMT[CMT_i].ppa = PBA2PPA + i; // Cache ppa, flag update
			CMT[CMT_i].flag = 1;
		}
		/* Please invalidate a page that CMT_i != -1 && ppa != origin_ppa[i] */
		/* If not, batch update */
		else{
			vba = INT32_MAX; // Initial state
			p_table = NULL;
			if(vba == D_IDX) // If page share same vba with previous page
				p_table[P_IDX].ppa = PBA2PPA + i; // Update mapping table
			else if(vba != INT32_MAX){ // Write mapping table to t_ppa
				tp_alloc(&t_ppa);
				temp_value_set2 = inf_get_valueset(temp_value_set->value, FS_MALLOC_W, PAGESIZE);
				__demand.li->push_data(t_ppa, PAGESIZE, temp_value_set2, 1, assign_pseudo_req()); // Write tpage to t_ppa
				inf_free_valueset(temp_value_set2, FS_MALLOC_W);
				demand_OOB[t_ppa] = (D_OOB){vba, 1}; // Update OOB of tpage
				demand_OOB[GTD[vba].ppa].valid_checker = 0; // Invalidate previous tpage
				GTD[vba].ppa = t_ppa; // GTD update to new tpage ppa
				inf_free_valueset(temp_value_set, FS_MALLOC_R);
				p_table = NULL;
			}
			if(p_table == NULL && i != valid_page_num){ // Start to make new tpage
				vba = D_IDX; // Get vba
				temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
				__demand.li->pull_data(GTD[vba].ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Load tpage from GTD[vba].ppa
				p_table = (D_TABLE*)temp_value_set->value; // p_table = mapping table in tpage
				p_table[P_IDX].ppa = PBA2PPA + i; // Update mapping table
			}
		}
	}

	/* Write dpages */
	for(int i = 0; i < valid_page_num; i++){
		SRAM_unload(PBA2PPA + i, i); 
	}
	DPA_status = PBA2PPA + valid_page_num;	// DPA_status update
}

/* ppa_compare
 * Used to sort pages as an order of ppa
 * Used in tpage_GC to do GC as an order of ppa of translation pages
 */
int ppa_compare(const void *a, const void *b){
	uint32_t num1 = (uint32_t)(((D_TABLE*)a)->ppa);
	uint32_t num2 = (uint32_t)(((D_TABLE*)a)->ppa);
	if(num1 < num2) return -1;
	else if(num1 == num2) return 0;
	else return 1;
}

/* Please enhance the full merge algorithm !!! */
/* tpage_GC
 * Find all translation pages using GTD and GC all of translation blocks
 * Copy GTD and them as an order of ppa of a translation page
 * Move as many as translation pages to one translation block that hold lowest ppa translation page
 * Repeat until no more translation pages remains to move
 */

void tpage_GC(){
	int n = 0;
	int32_t head_ppa;
	D_TABLE* GTDcpy = (D_TABLE*)malloc(GTDSIZE);/
	memcpy(GTDcpy, GTD, GTDSIZE); // Make copy of GTD
	qsort(GTDcpy, GTDENT, sizeof(D_TABLE), ppa_compare); // Sort GTDcpy by t_ppa order

	for(int i = 0; i < GTDENT; i++){
		if(GTDcpy[i].ppa == -1) // End of GTD
			break;
		else{ //
			SRAM_load(GTDcpy[i].ppa, n);
			if(n == 0)
				head_ppa = GTDcpy[i].ppa - GTDcpy[i].ppa % _PPB; // Remember ppa of first page in SRAM
			if(n == _PPB){
				__demand.li->trim_block(head_ppa, false); // Trim block of first page in SRAM
				for(int j = 0; j < _PPB; j++){ // Unload tpages to trimed block
					GTD[d_sram[j].OOB_RAM.reverse_table].ppa = head_ppa + j;
					SRAM_unload(head_ppa + j, j);
				}
				n = -1;
			}
			n++;
		}
	}
	if(n > 0){ // Move leftover tpages
		__demand.li->trim_block(head_ppa, false);
		for(int j = 0; j < n; j++){
			GTD[d_sram[j].OOB_RAM.reverse_table].ppa = head_ppa + j;
			SRAM_unload(head_ppa + j, j);
		}
		TPA_status = head_ppa + n; // Update TPA_status
		n = 0;
	}
	free(GTDcpy);
}

/* SRAM_load
 * Load a page located at ppa and its OOB to ith d_sram
 */
void SRAM_load(int32_t ppa, int idx){
	value_set *temp_value_set;

	temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	__demand.li->pull_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req()); // Load page in ppa
	d_sram[idx].valueset_RAM = temp_value_set; // Load page to d_sram
	d_sram[idx].OOB_RAM = demand_OOB[ppa];	// Load OOB to d_sram
	demand_OOB[ppa] = (D_OOB){-1, 0};	// OOB init
}

/* SRAM_unload
 * Write a page and its OOB in ith d_sram to correspond ppa
 */
void SRAM_unload(int32_t ppa, int idx){
	value_set *temp_value_set;

	temp_value_set = inf_get_valueset(d_sram[idx].valueset_RAM->value, FS_MALLOC_W, PAGESIZE); // Make valueset to WRITEMODE
	__demand.li->push_data(ppa, PAGESIZE, temp_value_set, 1, assign_pseudo_req());	// Unload page to ppa
	inf_free_valueset(temp_value_set, FS_MALLOC_W);
	inf_free_valueset(d_sram[idx].valueset_RAM, FS_MALLOC_R);
	demand_OOB[ppa] = d_sram[idx].OOB_RAM;	// Unload OOB to ppa
	d_sram[idx].valueset_RAM = NULL;	// SRAM init
	d_sram[idx].OOB_RAM = (D_OOB){-1, 0};
}

/* demand_GC
 * Determine which GC scheme to be use
 * If victim block is 'N' type, just trim the victim block and return true
 * If req block type is 'T', do tpage_GC and return true
 * If req block type is 'D' and victim block is 'D' type with at least one more invalid pages, do dpage_GC and return true
 * Otherwise, return false
 */
bool demand_GC(char btype){
	int32_t PBA2PPA = (PBA_status % _NOB) * _PPB; // Change PBA to PPA
	char victim_btype;
	victim_btype = btype_check();
	/* Case 0 : GC block type 'N' */
	if(victim_btype == 'N'){
		__demand.li->trim_block(PBA2PPA, false);
		if(btype == 'T')
			TPA_status = PBA2PPA;
		else
			DPA_status = PBA2PPA;
		return true;
	}
	else{
		/* Case 1 : GC block type 'T' */
		if(btype == 'T'){
			tpage_GC(); // Find t_blocks and GC all of them
			return true;
		}
		/* Case 2 : GC block type 'D' */
		else{
			/* Check GCability */
			if(victim_btype != 'D')
				return false; // Cannot do GC b/c victim is t_block
			for(int i = PBA2PPA; i < PBA2PPA + _PPB; i++){ // Check existance of invalid pages
				if(!demand_OOB[i].valid_checker)
					break;
				else if(i == PBA2PPA + _PPB - 1)
					return false;
			}
			/* Data block GC */
			dpage_GC();	// Do data block GC
			return true;
		}
	}
	return false;
}
#endif
