#include "demand_hash.h"

static uint32_t hashing(hash_t *ht_ptr, uint32_t key){
	uint32_t res = key;
	res = (res+0x7ed55d16) + (res<<12);
	res = (res^0xc761c23c) ^ (res>>19);
	res = (res+0x165667b1) + (res<<5);
	res = (res+0xd3a2646c) ^ (res<<9);
	res = (res+0xfd7046c5) + (res<<3);
	res = (res^0xb55a4f09) ^ (res>>16);
	res = res % ht_ptr->size;
	return res;
}

/*
 * hash_init () - Initialize a new hash table
 *
 */
void hash_init(hash_t *ht_ptr, int buckets){
	//Set default bucket size
	if(buckets==0)
		buckets = 16;

	/* initialize the table */
	ht_ptr->entries = 0;
	ht_ptr->size = 2;
	/* ensure buckets is a power of 2 */
	while(ht_ptr->size < buckets){
		ht_ptr->size <<= 1;
	}

	/* allocate memory for table */
	ht_ptr->bucket = (hash_node **)calloc(ht_ptr->size, sizeof(hash_node));
	ht_ptr->bucket_cnt = (int32_t *)calloc(ht_ptr->size, sizeof(int32_t));
	//ht_ptr->bucket = (hash_node **)malloc(sizeof(hash_node *) * ht_ptr->size);
	return ;
}
/*
 * hash_rebuild_table () - Create new hash table when old one fills up.
 *
 */

int32_t hash_rebuild_table(hash_t *ht_ptr){
	hash_node **old_bucket, *old_hash, *tmp;
	int32_t *old_bucket_cnt;
	int32_t old_size, h, i;
	int32_t bucket_idx;
	int32_t b_form_size = 0;
	bool flag = 0;

	old_bucket     = ht_ptr->bucket;	
	old_bucket_cnt = ht_ptr->bucket_cnt;
	old_size       = ht_ptr->size;
	/*create a new table and rehash old buckets */
	//Resize is 2x of existing hash size
	hash_init(ht_ptr, old_size << 1);

	b_form_size = BITMAP_SIZE + (ht_ptr->size * P_SIZE);	

	for(i = 0 ; i < old_size; i++){
		old_hash = old_bucket[i];
		while(old_hash){
			tmp = old_hash;
			old_hash = old_hash->next;
			h = hashing(ht_ptr, tmp->key);
			tmp->next = ht_ptr->bucket[h];
			ht_ptr->bucket[h] = tmp;
			ht_ptr->bucket_cnt[h]++;
			ht_ptr->entries++;

			b_form_size += NODE_SIZE;
			if(b_form_size > CHANGE_SIZE){
				flag = 1;
				break;
			}
		}
		
		if(flag) break;
	}
	free(old_bucket);
	free(old_bucket_cnt);

	if(flag){
		return HASH_FULL;
	}
	
	return HASH_REBUILD;


}

/*
 * hash_lookup() - Lookup an entry in the hash table and return a data to
 *          it or HASH_FAIL if it wasn't found
 */

hash_node *hash_lookup(hash_t *ht_ptr, uint32_t key){
	int32_t h;
	hash_node *node;
	h = hashing(ht_ptr, key);
	for(node = ht_ptr->bucket[h]; node != NULL; node = node->next){
		if(node->key == key)
			return node;
	}

	return NULL;
}

/*
 * hash_insert() - Insert an entry into the hash table. If the entry already exists
 * return HASH_UPDATE after update it, otherwise return HASH_SUCCESS
 *
 */

int32_t hash_insert(hash_t *ht_ptr, uint32_t key, int32_t data){
	int32_t tmp, h;
	hash_node *node;
	int32_t res;
		

	/* check to see if the entry exists */
	/* if the entry already exists, return HASH_UPDATE. Otherwise return a HASH_SUCCESS */
	/*
	if((node=hash_lookup(ht_ptr, key)) != NULL){
		node->key = key;
		node->data = data;
		return HASH_UPDATE;
	}
	*/
	/* expand the table if needed */
	while(ht_ptr->entries >= HASH_LIMIT*ht_ptr->size){
		res = hash_rebuild_table(ht_ptr);
		if(res == HASH_FULL)
			return HASH_FAIL;	
	}

	h = hashing(ht_ptr, key);
	node = (hash_node *)malloc(sizeof(hash_node));
	node->key = key;
	node->data = data;
	node->next = ht_ptr->bucket[h];
	ht_ptr->bucket[h] = node;
	ht_ptr->bucket_cnt[h]++;
	ht_ptr->entries++;

	if(res == HASH_REBUILD)
		return HASH_REBUILD;

	return HASH_SUCCESS;


	/* insert the new entry */

}
/*
 * hash_delete() - Remove an entry from a hash table and return a pointer
 * to its data or HASH_FAIL if it wasn't found
 *
 */

int32_t hash_delete(hash_t *ht_ptr, uint32_t key){
	hash_node *node, *last;
	int32_t data;
	int32_t h;

	/* find the node to remove */
	h = hashing(ht_ptr, key);
	for(node = ht_ptr->bucket[h]; node; node = node->next){
		if(node->key == key)
			break;
	}

	/* Didn't find anything, return HASH_FAIL */
	if(node == NULL)
		return HASH_FAIL;

	/* if node is at head of bucket, we have it easy */
	if(node==ht_ptr->bucket[h]){
		ht_ptr->bucket[h] = node->next;
	}else{
		/* find the previous node before the node we want to remove */
		for(last = ht_ptr->bucket[h]; last && last->next; last = last->next){
			if(last->next == node)
				break;
		}
		last->next = node->next;
	}

	ht_ptr->entries--;
	ht_ptr->bucket_cnt[h]--;
	/* free memory and return the data */
	
	data = node->data;
	free(node);

	return data;
}

/* hash_entries() - return the number of hash table entries.
 *
 */
int32_t hash_entries(hash_t *ht_ptr){
	return ht_ptr->entries;
}

int32_t hash_buckets(hash_t *ht_ptr){
	if(ht_ptr->bucket_cnt == NULL){
		return HASH_FAIL;
	}

	for(int i = 0; i < ht_ptr->size; i++){
		printf("bucket[%d] = %d\n",i,ht_ptr->bucket_cnt[i]);
	}

	return HASH_SUCCESS;
}



/*
 * hash_destroy() - Delete all remaining entries.
 *
 *
 */

void hash_destroy(hash_t *ht_ptr){
	hash_node *node, *last;
	int i;

	for(i = 0; i < ht_ptr->size; i++){
		node = ht_ptr->bucket[i];
		while(node != NULL){
			last = node;
			node = node->next;
			free(last);
		}
		ht_ptr->bucket_cnt[i] = 0;
	}
	ht_ptr->entries = 0;
	ht_ptr->size = 0;
	/* free the entire array of buckets */
	
	if(ht_ptr->bucket != NULL){
		free(ht_ptr->bucket);
		free(ht_ptr->bucket_cnt);
		memset(ht_ptr, 0, sizeof(hash_t));
	}
	

	return ;
}


/*
 * alos () - Find the average length of search
 *
 */

static float alos(hash_t *ht_ptr){
	int i, j;
	float alos = 0;
	hash_node *node;

	for(i = 0 ; i < ht_ptr->size; i++){
		for(node=ht_ptr->bucket[i], j = 0; node != NULL; node=node->next, j++){
			if(j)
				alos += ((j*(j+1))>>1);
		}
	}

	return (ht_ptr->entries ? alos/ht_ptr->entries : 0);

}

char *hash_stats(hash_t *ht_ptr){
	static char buf[1024];
	sprintf(buf, "%u slots, %u entries, and %1.2f ALOS",(int)ht_ptr->size, (int)ht_ptr->entries, alos(ht_ptr));
	return buf;
}







