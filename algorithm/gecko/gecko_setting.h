#ifndef __H_GECKO_SET__
#define __H_GECKO_SET__

#define NOB 2048
#define PPB 256
#define NOP (NOB*256)
#define PAGESIZE 8192
#define KEYT uint32_t
#define PTR char*

#define MAX_L 30 //max level number
#define PROB 4 //the probaility of level increasing : 1/PROB => 1/4
#define BITMAP uint8_t
#define ERASET uint8_t //erase type
#define BM_RANGE 32
#define T_value 2
#define GE_SIZE 37

#endif

