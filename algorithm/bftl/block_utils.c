#include "block.h"

algo_req* assign_pseudo_req(TYPE type, value_set *temp_v, request *req){
	algo_req *pseudo_my_req = (algo_req*)malloc(sizeof(algo_req));
	block_params *params = (block_params*)malloc(sizeof(block_params));
	pseudo_my_req->parents = req;
	params->type = type;
	params->value = temp_v;
	pseudo_my_req->end_req = block_end_req;
	pseudo_my_req->params = (void*)params;
	return pseudo_my_req;
}

/* Set set_pointer to first-meet ERASE index from current set_pointer like a round-robin */
int32_t block_findsp(int32_t checker)
{
	set_pointer = 0;

	for (; set_pointer < nob; ++set_pointer) {
		if (block_valid_array[set_pointer] == ERASE) {
			checker = 1;
			break;
		}
	}
	if (checker == 0) {
		for (set_pointer =0; set_pointer < nob; ++set_pointer)
			if (block_valid_array[set_pointer] == ERASE){
				checker = 1;
				break;
			}
	}
	return checker;
}

/* Check Last offset */
int8_t block_CheckLastOffset(uint32_t* lastoffset_array, uint32_t PBA, uint32_t offset)
{
	if (lastoffset_array[PBA] <= offset) {
		lastoffset_array[PBA] = offset;
		return 1;
	}
	else // Moving Block with target offset update
		return 0;
}

value_set* SRAM_load(uint32_t i, uint32_t old_PPA_zero)
{
	algo_req* temp_req = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set = inf_get_valueset(NULL, FS_MALLOC_R, PAGESIZE);
	temp_req->parents = NULL;
	temp_req->end_req = block_algo_end_req;
	__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_value_set, ASYNC, temp_req);
	return temp_value_set;
}

void SRAM_unload(uint32_t i, uint32_t new_PPA_zero)
{
	algo_req* temp_req2 = (algo_req*)malloc(sizeof(algo_req));
	value_set* temp_value_set2 = inf_get_valueset(sram_valueset[i].value, FS_MALLOC_W, PAGESIZE);
	temp_req2->parents = NULL;
	temp_req2->end_req = block_algo_end_req;
	__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_value_set2, ASYNC, temp_req2);
	inf_free_valueset(temp_value_set2, FS_MALLOC_W);
}
