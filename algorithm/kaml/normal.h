#ifndef NORMAL_H
#define NORMAL_H
#include "../../include/container.h"
typedef struct normal_params {
	int cnt;
	int finding; //0:initial try, 1:no key, 2:same key 3:different key
	int temp;
}normal_params;

typedef struct hash_req {
	uint32_t ppa;
	uint32_t hash_key;
	request *parents;
	MeasureTime latency_lower;
	uint8_t type;
	bool rapid;
	uint8_t type_lower;
	//0: normal, 1 : no tag, 2: read delay 4:write delay

	void *(*end_req)(struct hash_req *const);
	void *params;
}hash_req;

uint32_t normal_create (lower_info*, algorithm *);
void normal_destroy (lower_info*,  algorithm *);
uint32_t normal_get(request *const);
uint32_t normal_set(request *const);
uint32_t normal_remove(request *const);
void *normal_end_req(hash_req*);
uint32_t hashing_key(char*,uint8_t);
#endif
