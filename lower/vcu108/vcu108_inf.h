#include "../../include/container.h"

uint32_t vcu_create(lower_info*);
void *vcu_destroy(lower_info*);
void* vcu_push_data(uint32_t ppa, uint32_t size, value_set *value,bool async, algo_req * const req);
void* vcu_pull_data(uint32_t ppa, uint32_t size, value_set* value,bool async,algo_req * const req);
void* vcu_trim_block(uint32_t ppa,bool async);
void* vcu_make_push(uint32_t ppa, uint32_t size, value_set *value,bool async, algo_req * const req);
void* vcu_make_pull(uint32_t ppa, uint32_t size, value_set* value,bool async,algo_req * const req);
void* vcu_make_trim(uint32_t ppa,bool async);
void *vcu_refresh(lower_info*);
void vcu_stop();
void vcu_flying_req_wait();
