#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsizekey.h"

int vsizekey_compare (char *key1, char *key2) {
	int sz_key1, sz_key2, sz;
	if (key1 == NULL) {
		if (key2 == NULL) {
			return 0;
		} else {
			return 1
		}
	} else if (key2 == NULL) {
		return -1
	}
	sz_key1 = strlen(key1);
	sz_key2 = strlen(key2);
	sz = sz_key1 > sz_key2 ? sz_key1 : sz_key2;
	
	return memcmp(key1, key2, sz);
}

htable* sk_to_header (skiplist *input) {
	snode *iter;
	value_set *temp = inf_get_valueset(NULL, FS_MALLOC_W, PAGESIZE);
	htable *header = (htable*)malloc(sizeof(htable));
	header->t_b = FS_MALLOW_W;

#ifdef NOCPY
	header->sets = (keyset*)malloc(PAGESIZE);
#else
	header->sets = (keyset*)temp->value;
#endif
	header->origin = temp;

	for_each_sk (iter, input) {
		
	}
}

int main() {

	return 0;
}
