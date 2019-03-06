
/*
 * Demand FTL Header File
 */

#ifndef __DEMAND_H__
#define __DEMAND_H__

#include "../../include/container.h"
#include "../../include/setting.h"

#define DEMAND_EXIT_DATAR    0
#define DEMAND_EXIT_DATAW    1
#define DEMAND_EXIT_MAPPINGR 2
#define DEMAND_EXIT_MAPPINGW 3
#define DEMAND_EXIT_NOTFOUND 4
#define DEMAND_EXIT_WBHIT    5
#define DEMAND_EXIT_WBFLUSH  6
// Reserve DATAW + WBFLUSH   7 (1+6)


/* Structures */
struct demand_handler {

};

struct demand_env {

};

/* Functions */

// ++ demand.c
uint32_t demand_create(lower_info*, algorithm*);
void     demand_destroy(lower_info*, algorithm*);
uint32_t demand_read(request *const);
uint32_t demand_write(request *const);
uint32_t demand_remove(request *const);
// -- demand.c

// ++ demand_internal.c
int __demand_create(lower_info*, algorithm*);
int __demand_destroy(lower_info*, algorithm*);
int __demand_read(request *const);
int __demand_write(request *const);
int __demand_remove(request *const);
// -- demand_internal.c

// ++ gc.c
int data_gc();
int trns_gc();
// -- gc.c

#endif
