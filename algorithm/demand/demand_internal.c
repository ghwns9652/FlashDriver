
/*
 * Demand FTL internal functions
 */

#include "demand.h"
#include "cache.h"

static lower_info *__li;
static algorithm *__algo;
static struct cache_module *__cache;

int __demand_create(lower_info *li, algorithm *algo, struct cache_module *cache) {
	__li    = li;
	__algo  = algo;
	__cache = cache;
}

int __demand_destory() {

}

int __demand_read(request *const req) {
	int rc;
	int32_t lpa, ppa;

	bench_algo_start(req);
	lpa = req->key;

	// Check cache
	if (!__cache->is_hit(lpa)) {
		if (__cache->is_full(req->type)) {
			rc = __cache->evict(__li, req);
			if (rc > 0) { // cf) number of evicted pages
				rc = DEMAND_EXIT_MAPPINGW;
				goto exit;
			}
		}
		rc = __cache->load(__li, req);
		if (rc > 0) { // cf) number of loaded pages
			rc = DEMAND_EXIT_MAPPINGR;
			goto exit;
		}
	}

	// Read actual data
	ppa = __cache->get_ppa(lpa); // TODO: do cache-update in this function
	__li->read(ppa, PAGESIZE, req->value, ASYNC, make_algo_req(DATAR, NULL, req));
	rc = DEMAND_EXIT_DATAR;

exit:
	bench_algo_end(req);
	return rc;
}

int __demand_write(request *const req) {
	int rc;
	int32_t lpa, ppa;

	bench_algo_start(req);

	if (write_buffer == max_wb) {
		for (int i = 0; i < max_wb; i++) {
			__li->write(ppa_prefetch[i],
				    PAGESIZE,
				    wb_entry->value,
				    ASYNC,
				    make_algo_req(DATAW, wb_entry->value, NULL);
		}
	}

	lpa = req->key;
exit:
	bench_algo_end(req);
	return rc;
}

int __demand_remove(request *const req) {

}

void *demand_end_req(algo_req *a_req) {

}
