#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsizekey.h"

int vsizekey_cmp (char *key1, char *key2) {
	int sz_key1, sz_key2, sz;
	if (key1 == NULL) {
		if (key2 == NULL) {
			return 0;
		} else {
			return 1;
		}
	} else if (key2 == NULL) {
		return -1;
	}
	sz_key1 = strlen(key1);
	sz_key2 = strlen(key2);
	sz = sz_key1 > sz_key2 ? sz_key1 : sz_key2;
	
	return memcmp(key1, key2, sz);
}

htable* sk_to_header (skiplist *input) {
	snode *iter;
	vsheader *vsh;
	value_set *temp = inf_get_valueset(NULL, FS_MALLOC_W, PAGESIZE);
	htable *header = (htable*)malloc(sizeof(htable));	// mush be freed later
	header->t_b = FS_MALLOW_W;

#ifdef NOCPY
	header->sets = (keyset*)malloc(PAGESIZE);
#else
	header->sets = (keyset*)temp->value;
#endif
	header->origin = temp;

	vsh = header->sets;	// must be modified //
	vsh->key_cnt = 0;
	vsh->size = sizeof(key_cnt) + sizeof(map_t) * input->size;

	for_each_sk (iter, input) {
		header_insert(header, iter);	
	}

	return header;
}

htable *hash_mem_cvt2table(skiplist *mem,run_t* input){
	value_set *temp=inf_get_valueset(NULL,FS_MALLOC_W,PAGESIZE);
	htable *res=(htable*)malloc(sizeof(htable));
	res->t_b=FS_MALLOC_W;

#ifdef NOCPY
	res->sets=(keyset*)malloc(PAGESIZE);
#else
	res->sets=(keyset*)temp->value;
#endif
	res->origin=temp;

#ifdef BLOOM
	BF *filter=bf_init(h_max_table_entry(),LSM.disk[0]->fpr);
	input->filter=filter;
#endif
	input->cpt_data=res;

	snode *target;
	keyset t_set;
	hash *t_hash=(hash*)res->sets;
	memset(res->sets,-1,PAGESIZE);
	t_hash->n_num=t_hash->t_num=0;

	int idx=0;
	for_each_sk(target,mem){
		t_set.lpa=target->key;
		t_set.ppa=target->isvalid?target->ppa:UINT_MAX;
#ifdef BLOOM
		bf_set(filter,t_set.lpa);
#endif
		idx++;
		hash_body_insert(t_hash,t_set);
	}
	return res;
}

int main() {

	return 0;
}
