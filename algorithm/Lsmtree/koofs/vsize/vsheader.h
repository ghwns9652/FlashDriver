#ifndef __VSHEADER_H__
#ifndef __VSHEADER_H__

#include "skiplist.h"
#include "level.h"

#define SIZET uint16_t
#define MAPSIZE 1024

#define for_each_vsh(node, vsh)\
	for(int i=0, node=vsh->map[0];\
			i<vsh->key_cnt;\
			node=vsh->map[i++])
			
#if 0
typedef struct map_t{
	char *addr;
	KEYT ppa;
	uint8_t size;
}map_t;
#endif

typedef struct vsheader{
	SIZET key_cnt;
	int size;
	keyset *map;
}vsheader;

/* 
   vsizekey_compare(key1, key2)
   return value
   > 0  : key1 is big
   == 0 : key1 == key2
   < 0  : key2 is big
*/
int vsizekey_compare (char *key1, char *key2);
void sk_to_header(skiplist *input);
void sk_to_header();
void sk_to_header();

#endif
