
/*
 * Demand FTL exposed functions
 */

#include "demand.h"

#if defined(COARSE_GRAINED)
#include "./cache/cg_cache/cache.h"
#elif deinfed(FINE_GRIAINED)
#include "./cache/fg_cache/cache.h"
#elif defined(S_FTL)
#include "./cache/s_cache/cache.h"
#elif defined(TP_FTL)
#include "./cache/tp_cache/cache.h"
#elif defined(HASH_FTL)
#include "./cache/hash_cache/cache.h"
#endif

algorithm algo_demand = {
    .create  = demand_create,
    .destroy = demand_destroy,
    .read    = demand_read,
    .write   = demand_write,
    .remove  = demand_remove
}

extern struct cache_module *cache;

uint32_t demand_create(lower_info *li, algorithm *algo) {
	int rc = __demand_create(li, algo, cache);
	return 0;
}

void demand_destroy(lower_info *li, algorithm *algo) {
	int rc = __demand_destroy(li, algo, cache);
	return 0;
}

uint32_t demand_read(request *const req) {
	int rc = __demand_read(req);
	return 0;
}

uint32_t demand_write(request *const req) {
	int rc = __demand_write(req);
	return 0;
}

uint32_t demand_remove(request *const req) {
	int rc = __demand_remove(req);
	return 0;
}

