#include "../../include/container.h"
typedef struct normal_params{
	request *parents;
	int test;
}normal_params;

typedef struct normal_cdf_struct{
	uint64_t total_micro;
	uint64_t cnt;
	uint64_t max;
	uint64_t min;
}n_cdf;

uint32_t tp_create (lower_info*, algorithm *);
void tp_destroy (lower_info*,  algorithm *);
uint32_t tp_get(request *const);
uint32_t tp_set(request *const);
uint32_t tp_remove(request *const);
void *tp_end_req(algo_req*);
