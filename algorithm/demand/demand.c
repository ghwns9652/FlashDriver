#include "demand.h"

algorithm algo_demand = {
    .create  = demand_create,
    .destroy = demand_destroy,
    .read    = demand_read,
    .write   = demand_write,
    .remove  = demand_remove
}

uint32_t demand_read(request *const req) {
	uint32_t ret = __demand_read(req);

	return DFTL_EXIT_SUCCESS;
}

