#ifndef _BM_INTERFACE_H_
#define _BM_INTERFACE_H_

#include "BM_common.h"
#include "BM_Block.h"
#include "BM_Heap.h"

void		BM_invalidate_ppa(Block* ptrBlock[], uint32_t PPA);
int32_t		BM_is_invalid_ppa(Block* ptrBlock[], uint32_t size, uint32_t PPA);
uint32_t	BM_get_gc_victim(Block *ptrBlock[], uint32_t size);
uint32_t	BM_get_weared_block(Block *ptrBlock[], uint32_t size);


#endif // !_BM_INTERFACE_H_
