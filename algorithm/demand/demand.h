
/*
 * Demand FTL Header File
 */

#ifndef __DEMAND_H__
#define __DEMAND_H__
#define A 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "../../include/container.h"
#include "../../include/setting.h"
#include "../../include/dftl_setting.h"

#include "caching.h"

#define DEMAND_EXIT_SUCCESS 0
#define DEMAND_EXIT_FAILURE 1

/* Structures */
struct demand_handle {
	// CMT
	// 
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
uint32_t __demand_create(lower_info*, algorithm*);
void     __demand_destroy(lower_info*, algorithm*);
uint32_t __demand_read(request *const);
uint32_t __demand_write(request *const);
uint32_t __demand_remove(request *const);
// -- demand_internal.c

// ++ gc.c
uint32_t data_gc();
uint32_t trans_gc();
// -- gc.c

#endif
