
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

static algo_req *make_algo_req(FS_TYPE type, PTR value, request *req) {
	algo_req *a_req = (algo_req *)malloc(sizeof(algo_req));

	return a_req;
}

int __demand_read(request *const req) {
	int rc;
	int32_t lpa, ppa;

	bench_algo_start(req);

	// Get LPA
	lpa = req->key;

	// Check write buffer
	wb_entry = skiplist_find(write_buffer, lpa);
	if (wb_entry) {
		rc = DEMAND_EXIT_WBHIT;
		goto exit;
	}

	// Check cache
	if (!__cache->is_hit(lpa, req->type)) {
		if (__cache->is_full(req->type)) {
			rc = __cache->evict(__li, req);
			if (rc > 0) { // cf) rc = number of evicted dirty pages (incur mapping writes)
				rc = DEMAND_EXIT_MAPPINGW;
				goto exit;
			}
		}

		rc = __cache->load(__li, req);
		if (rc > 0) { // cf) rc = number of loaded pages (incur mapping reads)
			rc = DEMAND_EXIT_MAPPINGR;
			goto exit;
		} else if (rc < 0) { // cf) rc = -1 means no mapping page was written to flash
			rc = DEMAND_EXIT_NOTFOUND;
			goto exit;
		}
	}

	// Read actual data
	ppa = __cache->get_ppa(lpa); // TODO: do cache-update in this function
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

static int flush_wb(skiplist *wb) {
	int i;
	snode   *wb_entry;
	sk_iter *iter = skiplist_get_iterator(wb);

	bool *updated = (bool *)malloc(max_wb * sizeof(bool));
	memset(updated, 0, max_wb * sizeof(bool));

	// Update mapping
	for (i = 0; i < max_wb; i++) {
		wb_entry = skiplist_get_next(iter);
		if (__cache->is_hit(wb_entry->key)) {
			__cache->update(wb_entry->key, DATAW);
			updated[i] = true;
		}
	}

	// TODO: mapping r/w

	// Flush all data in write buffer
	for (i = 0; i < max_wb; i++) {
		wb_entry = skiplist_get_next(iter);
		__li->write(ppa_prefetch[i],
			    PAGESIZE,
			    wb_entry->value,
			    ASYNC,
			    make_algo_req(DATAW, wb_entry->value, NULL));
	}

	// Prefetch PPAs to hide write overhead on read
	for (int i = 0; i < max_wb; i++) {
		ppa_prefetch[i] = dp_alloc();
	}

	// Reset the write buffer
	free(iter);
	skiplist_free(wb);
	wb = skiplist_init();

        // Wait until all flying requests(set) are finished
	__li->lower_flying_req_wait();

	return 0;
}

int __demand_write(request *const req) {
	int rc = 0;
	int32_t lpa, ppa;
	snode *wb_entry;

	bench_algo_start(req);

	// Check write buffer whether to flush
	if (write_buffer == max_wb) {
		flush_wb(write_buffer);
		rc = DEMAND_EXIT_FLUSH;
	}

	// Get LPA
	lpa = req->key;

	// Insert actual data to write buffer
	wb_entry = skiplist_insert(write_buffer, lpa, req->value, true);

	rc += DEMAND_EXIT_DATAW;

exit:
	bench_algo_end(req);
	return rc;
}

int __demand_remove(request *const req) {
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

