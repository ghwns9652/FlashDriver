#ifndef PFTL_H
#define PFTL_H

#include "../../include/container.h"

#define DATA_R 0
#define DATA_W 1
#define GC_R   2
#define GC_W   3

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

uint32_t pftl_create (lower_info*, algorithm *);
void pftl_destroy (lower_info*,  algorithm *);
uint32_t pftl_read(request *const);
uint32_t pftl_write(request *const);
uint32_t pftl_remove(request *const);
void *pftl_end_req(algo_req*);
#endif
