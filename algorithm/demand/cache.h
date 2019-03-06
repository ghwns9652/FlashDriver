#ifndef __DEMAND_CACHE_H__
#define __DEMAND_CACHE_H__

#include "demand.h"

struct cache_module {
	int (*create) (struct demand_env *); 
	int (*destroy) ();
	bool (*is_hit) (int lpa, int type);
	bool (*is_full) (int type);
	int (*evict) (lower_info *, request *);
	int (*load) (lower_info *, request *);
	int (*get_ppa) (int ppa);
	int (*update) (int lpa, int ppa);
};

#endif
