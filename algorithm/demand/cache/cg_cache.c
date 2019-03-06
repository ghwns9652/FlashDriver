
/*
 * Coarse-grained cache implementation
 */

#include "../demand.h"
#include "../cache.h"

struct cache_module cache = {
	.create  = cg_create,
	.destroy = cg_destroy,
	// methods
};

int cg_create(struct demand_env *env) {

}

int cg_destroy() {

}
