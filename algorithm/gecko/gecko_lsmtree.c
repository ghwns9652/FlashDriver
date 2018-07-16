#define _LARGEFILE64_SOURCE
#include "gecko_lsmtree.h"

level *level_init(int max_lv){
	level *res = (level *)malloc(sizeof(level) * (max_lv + 1));
	uint32_t lpa = 0;
	for(int i = 0; i <= max_lv; i++){
		res[i].cur_cap = 0;
		res[i].max_cap = pow(T_value, i);
		//res[i].array = (node **)malloc(sizeof(node *) * res[i].max_cap);
		res[i].array = (node *)malloc(sizeof(node) * res[i].max_cap);
		if(i > 0){
			for(int j = 0; j < res[i].max_cap; j++){
				res[i].array[j].LPA = lpa++;
			}
		}
	}
	return res;
}

void level_free(level *lv, int max_lv){
	for(int i = 0; i <= max_lv; i++){
		free(lv[i].array);
	}
	free(lv);
}

lsmtree *lsm_init(){
	lsmtree *res = (lsmtree *)malloc(sizeof(lsmtree));
	res->max_level = (int)ceil(log((double)_NOB / (double)MAX_PER_PAGE) / log(T_value));
	res->levels = level_init(res->max_level);
	res->buffer = skiplist_init();
	res->fd = open("./storage.data", O_RDWR|O_CREAT|O_TRUNC, 0666);
	if(res->fd == -1){
		printf("file open error!\n");
		exit(-1);
	}
	return res;
}

void lsm_free(lsmtree *lsm){
	skiplist_free(lsm->buffer);
	level_free(lsm->levels, lsm->max_level);
	close(lsm->fd);
	free(lsm);
}

void lsm_buf_update(lsmtree *lsm, KEYT key, uint8_t offset, ERASET flag){
	//printf("key %d\n", key);
	skiplist_insert(lsm->buffer, key, offset, flag);
	if(lsm->buffer->size == MAX_PER_PAGE){
		lsm_buffer_flush(lsm, skiplist_flush(lsm->buffer));
	}
}

void lsm_node_fwrite(lsmtree *lsm, int lv_off, int nd_off){
	uint32_t offset = lsm->levels[lv_off].array[nd_off].LPA;
	lseek64(lsm->fd, (off64_t)(PAGESIZE * offset), SEEK_SET);
	write(lsm->fd, lsm->levels[lv_off].array[nd_off].memptr, PAGESIZE);
	free(lsm->levels[lv_off].array[nd_off].memptr);
	lsm->levels[lv_off].array[nd_off].memptr = NULL;
}

void lsm_buffer_flush(lsmtree *lsm, node *data){
	//printf("flush\n");
	if(lsm->levels[1].cur_cap == lsm->levels[1].max_cap){
		lsm_merge(lsm, 1);
	}
	memcpy(&lsm->levels[1].array[lsm->levels[1].cur_cap], data, sizeof(node));
	free(data);
	lsm_node_fwrite(lsm, 1, lsm->levels[1].cur_cap++); //범위가 작은게 뒤로 가도 되는가? 일단은 최근값이 뒤로 들어가는것으로로
	lsm->buffer = skiplist_init();
}

void lsm_node_recover(lsmtree *lsm, int lv_off, int nd_off){
	uint32_t offset, loc = 0;
	PTR temp = (PTR)malloc(PAGESIZE);
	lsm->mixbuf = skiplist_init();
	snode* t;
	offset = lsm->levels[lv_off].array[nd_off].LPA;
	lseek64(lsm->fd, (off64_t)(PAGESIZE * offset), SEEK_SET);
	read(lsm->fd, temp, PAGESIZE);
	for(int i = 0; i < MAX_PER_PAGE; i++){
		t = (snode*)&temp[loc];
		if(t->key == 0 && t->VBM[0] == 0 && t->VBM[1] == 0 && t->VBM[2] == 0 && t->VBM[3] == 0 && t->erase == 0){
			break;
		}
		skiplist_merge_insert(lsm->mixbuf, t);
		loc += sizeof(uint64_t) * 4 + sizeof(KEYT) + sizeof(ERASET);
	}
	free(temp);
	skiplist_dump_key_value(lsm->mixbuf);
	skiplist_free(lsm->mixbuf);
}

int lsm_merge(lsmtree *lsm, int lv_off){ //뒤에부터 들어가야됨 일단은 전부 꺼내서 전부 merge하는것으로로
	printf("merge start w/ level %d\n", lv_off);
	if(lsm->levels[lv_off + 1].cur_cap == lsm->levels[lv_off + 1].max_cap){
		if(lv_off + 1 != lsm->max_level){
			lsm_merge(lsm, lv_off + 1);
		}
	}
	uint32_t offset, loc;
	PTR temp = (PTR)malloc(PAGESIZE);
	lsm->mixbuf = skiplist_init();
	for(int i = 0; i < lsm->levels[lv_off].cur_cap; i++){
		loc = 0;
		offset = lsm->levels[lv_off].array[lsm->levels[lv_off].cur_cap - 1 - i].LPA;
		lseek64(lsm->fd, (off64_t)(PAGESIZE * offset), SEEK_SET);
		read(lsm->fd, temp, PAGESIZE);
		for(int j = 0; j < MAX_PER_PAGE; j++){ // 221개 전부 차있다고 가정하고 작성
			skiplist_merge_insert(lsm->mixbuf, (snode*)&temp[loc]);
			loc += sizeof(uint64_t) * 4 + sizeof(KEYT) + sizeof(ERASET);
		}
	}
	lsm->levels[lv_off].cur_cap = 0;
	//lv_off + 1
	if(lsm->levels[lv_off + 1].cur_cap > 0){
		for(int i = 0; i < lsm->levels[lv_off + 1].cur_cap; i++){
			offset = lsm->levels[lv_off + 1].array[lsm->levels[lv_off + 1].cur_cap - 1 - i].LPA;
			lseek64(lsm->fd, (off64_t)(PAGESIZE * offset), SEEK_SET);
			loc = 0;
			read(lsm->fd, temp, PAGESIZE);
			for(int j = 0; j < MAX_PER_PAGE; j++){
				skiplist_merge_insert(lsm->mixbuf, (snode*)&temp[loc]);
				loc += sizeof(uint64_t) * 4 + sizeof(KEYT) + sizeof(ERASET);
			}
			lseek64(lsm->fd, -(off64_t)(PAGESIZE * 2), SEEK_CUR);
		}
		lsm->levels[lv_off + 1].cur_cap = 0;
	}
	free(temp);
	//fwrite 미완
	KEYT key = lsm->mixbuf->start;
	KEYT end;
	printf("s %ld\n", lsm->mixbuf->size);
	printf("i %ld\n", lsm->mixbuf->size/MAX_PER_PAGE);
	for(int i = 0; i <= lsm->mixbuf->size/MAX_PER_PAGE; i++){
		lsm->levels[lv_off + 1].array[i].min = key;
		lsm->levels[lv_off + 1].array[i].memptr = skiplist_lsm_merge(lsm->mixbuf, key, &key, &end);
		lsm->levels[lv_off + 1].array[i].max = end;
		if(i == lsm->mixbuf->size/MAX_PER_PAGE){
			lsm->levels[lv_off + 1].array[i].max = lsm->mixbuf->end;
		}
		lsm->levels[lv_off + 1].cur_cap++;
		lsm_node_fwrite(lsm, lv_off + 1, i);
	}
	printf("merge end\n");
	//skiplist_dump_key_value(lsm->mixbuf);
	skiplist_free(lsm->mixbuf);
	return 1;
}

void print_level_status(lsmtree* lsm){
	for(int i = 1; i <= lsm->max_level; i++){
		if(lsm->levels[i].cur_cap > 0){
			printf("level %d's cur_cap is %d\n", i, lsm->levels[i].cur_cap);
		}
	}
}
