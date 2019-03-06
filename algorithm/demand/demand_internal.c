
/*
 * Demand FTL internal functions
 */

#include "demand.h"
#include "cache.h"

lower_info *__li;
algorithm *__algo;
struct demand_handler *demand;
struct demand_env *env;
extern struct cache_module cache;

static int *dh_init(struct demand_handler **handler, struct demand_env **env)
	// TODO: init dh members

	return dh;
}

static int dh_free() {
	// TODO: free dh members
	return 0;
}

int __demand_create(lower_info *li, algorithm *algo) {
	__li    = li;
	__algo  = algo;

	demand_init(&demand, &env);
	cache.create(env);
}

int __demand_destory() {
	cache.destroy();
	demand_free();
}

static algo_req *make_algo_req(FS_TYPE type, PTR value, request *req) {
	algo_req *a_req = (algo_req *)malloc(sizeof(algo_req));

	return a_req;
}

int __demand_read(request *const req) {
	int rc;
	int32_t lpa, ppa;

	skiplist *wb = demand->write_buffer;
	snode *wb_entry;

	bench_algo_start(req);

	// Get LPA
	lpa = req->key;

	// Check write buffer
	wb_entry = skiplist_find(wb, lpa);
	if (wb_entry) {
		rc = DEMAND_EXIT_WBHIT;
		goto exit;
	}

	// Check cache
	if (!cache.is_hit(lpa, req->type)) {
		if (cache.is_full(req->type)) {
			rc = cache.evict(__li, req);
			if (rc > 0) { // cf) rc = number of evicted dirty pages (incur mapping writes)
				rc = DEMAND_EXIT_MAPPINGW;
				goto exit;
			}
		}

		rc = cache.load(__li, req);
		if (rc > 0) { // cf) rc = number of loaded pages (incur mapping reads)
			rc = DEMAND_EXIT_MAPPINGR;
			goto exit;
		} else if (rc < 0) { // cf) rc = -1 means no mapping page was written to flash
			rc = DEMAND_EXIT_NOTFOUND;
			goto exit;
		}
	}

	// Read actual data
	ppa = cache.get_ppa(lpa); // TODO: do cache-update in this function
	if (ppa < 0) {
		rc = DEMAND_EXIT_NOTFOUND;
		goto exit;
	}
	__li->read(ppa, PAGESIZE, req->value, ASYNC, make_algo_req(DATAR, NULL, req));
	rc = DEMAND_EXIT_DATAR;

exit:
	bench_algo_end(req);
	return rc;
}

// TODO! mapping update
// cache.update(lpa, ppa);
// cache.is_hit
// cache.is_full
// cache.evict
static int flush_wb(skiplist *wb) {
	int i;

	snode   *wb_entry;
	sk_iter *iter = skiplist_get_iterator(wb);

	// Flush all data in write buffer
	for (i = 0; i < demand->max_wb; i++) {
		wb_entry = skiplist_get_next(iter);
		__li->write(ppa_prefetch[i],
			    PAGESIZE,
			    wb_entry->value,
			    ASYNC,
			    make_algo_req(DATAW, wb_entry->value, NULL));
	}

	// Prefetch PPAs to hide write overhead on read
	for (int i = 0; i < demand->max_wb; i++) {
		ppa_prefetch[i] = dp_alloc();
	}

	// Reset the write buffer
	free(iter);
	skiplist_free(wb);
	wb = skiplist_init();

	// Wait until all flying requests are finished
	__li->lower_flying_req_wait();

	return 0;
}

int __demand_write(request *const req) {
	int rc = 0;
	int32_t lpa, ppa;

	skiplist *wb = demand->write_buffer;
	snode *wb_entry;

	bench_algo_start(req);

	// Check write buffer whether to flush
	if (wb.size == demand->max_wb) {
		flush_wb(wb);
		rc = DEMAND_EXIT_WBFLUSH;
	}

	// Get LPA
	lpa = req->key;

	// Insert actual data to write buffer
	wb_entry = skiplist_insert(wb, lpa, req->value, true);

	rc += DEMAND_EXIT_DATAW;

exit:
	bench_algo_end(req);
	return rc;
}

int __demand_remove(request *const req) {
	// TODO -> later
	return 0;
}

void *demand_end_req(algo_req *a_req) {
	request *req = a_req->parent;

	switch (req->type) {
	case DATAR:
		if (req) {
			req->end_req(req);
		}
		break;

	case DATAW:
		if (req) {
			req->end_req(req);
		}
		break;

	case MAPPINGR:
		break;

	case MAPPINGW:
		break;

	case GCDR:
		break;

	case GCDW:
		break;

	case GCMR:
		break;

	case GCMW:
		break;
	}

	free(a_req);
	return NULL;
}

