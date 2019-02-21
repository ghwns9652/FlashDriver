#ifndef __BLOCK_H__
#define __BLOCK_H__

#define NUM_BLOCK _NOB
#define NUM_PAGE _NOP

#define NUM_T_BLOCK 1
#define NUM_D_BLOCK (NUM_BLOCK - NUM_T_BLOCK)

#define BLOCK_MAKE_SUCCESS 0
#define BLOCK_MAKE_FAIL    1


struct block{

};


int32_t block_init();

#endif
