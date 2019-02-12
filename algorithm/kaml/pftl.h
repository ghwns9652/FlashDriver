#ifndef PFTL_H
#define PFTL_H
#include "../../include/container.h"
#include "normal.h"

typedef struct pftl_params {
	request *parents;
	int test;
}pftl_params;

typedef struct algo_params {
	hash_req *parents;
	value_set *value;
}algo_params;

typedef struct normal_cdf_struct{
	uint64_t total_micro;
	uint64_t cnt;
	uint64_t max;
	uint64_t min;
}n_cdf;

uint32_t pftl_create (lower_info*, algorithm *);
void pftl_destroy (lower_info*,  algorithm *);
uint32_t pftl_read(hash_req *const);
uint32_t pftl_write(hash_req *const);
uint32_t pftl_remove(hash_req *const);
void *pftl_end_req(algo_req*);
bool exist(uint32_t);
#endif
