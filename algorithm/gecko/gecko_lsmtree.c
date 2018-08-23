#define _LARGEFILE64_SOURCE
#include "gecko.h"

int32_t num_page;
int32_t num_block;
int32_t p_p_b;
int32_t gepp; // Gecko entry per page

lsmtree *lsm_init(){
	// initialize all value by using macro.
	num_page = NOP;
	num_block = NOB;
	p_p_b = PPB;
	gepp = PAGESIZE / GE_SIZE;

	lsmtree *res = (lsmtree *)malloc(sizeof(lsmtree));
	res->max_level = (int)ceil(log((double)num_block / (double)gepp) / log((double)T_value));
	res->levels = level_init(res->max_level);
	res->buffer = skiplist_init();
	res->fd = open("./storage.data", O_RDWR|O_CREAT|O_TRUNC, 0666);
	if(res->fd == -1){
		printf("file open error!\n");
		exit(-1);
	}

	printf("!!! print info !!!\n");
	printf("page per block: %d\n", p_p_b);
	printf("number of block: %d\n", num_block);
	printf("number of page: %d\n", num_page);
	printf("GeckoEntry per Page: %d\n", gepp);
	printf("Max level: %d\n", res->max_level);
	printf("!!! print info !!!\n");
	exit(1);
	
	return res;
}

void lsm_free(lsmtree *lsm){
	skiplist_free(lsm->buffer);
	level_free(lsm->levels, lsm->max_level);
	close(lsm->fd);
	free(lsm);
}

level *level_init(int max_lv){
	level *res = (level *)malloc(sizeof(level) * (max_lv + 1));
	uint32_t lpa = 0;
	for(int i = 0; i <= max_lv; i++){
		res[i].cur_cap = 0;
		res[i].max_cap = pow(T_value, i);
		res[i].array = (node *)malloc(sizeof(node) * res[i].max_cap);
		for(int j = 0; j < res[i].max_cap; j++){
			res[i].array[j].LPA = lpa++;
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

void lsm_buf_update(lsmtree *lsm, KEYT key, uint8_t offset, ERASET flag){
	skiplist_insert(lsm->buffer, key, offset, flag);
	if(lsm->buffer->size == gepp){
		lsm_buffer_flush(lsm, skiplist_flush(lsm->buffer));
		lsm->buffer = skiplist_init();
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
	memcpy(&lsm->levels[0].array[lsm->levels[0].cur_cap], data, sizeof(node));
	free(data);
	lsm_node_fwrite(lsm, 1, lsm->levels[1].cur_cap++); //범위가 작은게 뒤로 가도 되는가? 일단은 최근값이 뒤로 들어가는것으로로
	if(lsm->levels[1].cur_cap == lsm->levels[1].max_cap){
		//lsm_merge(lsm, 1);
	}
}

void lsm_node_recover(lsmtree *lsm, int lv_off, int nd_off){
	snode* t;
	uint32_t offset, loc = 0;
	PTR temp = (PTR)malloc(PAGESIZE);
	offset = lsm->levels[lv_off].array[nd_off].LPA;
	lseek64(lsm->fd, (off64_t)(PAGESIZE * offset), SEEK_SET);
	read(lsm->fd, temp, PAGESIZE);
	for(int i = 0; i < gepp; i++){
		t = (snode*)&temp[loc];
		if(t->key == UINT32_MAX){
			break;
		}
		printf("key(%u): ", t->key);
		for(int i = 0; i < BM_RANGE; i++){
			printf("hexvalue%i: %d\n", i, t->VBM[i]);
		}
		printf("erase(%d): ", t->erase);
		loc += sizeof(BITMAP) * BM_RANGE + sizeof(KEYT) + sizeof(ERASET);
	}
	free(temp);
}

// 고쳐야됨
int lsm_merge(lsmtree *lsm, int lv_off){ //뒤에부터 들어가야됨 일단은 전부 꺼내서 전부 merge하는것으로로
	printf("merge start w/ level %d\n", lv_off);
	if(lsm->levels[lv_off + 1].cur_cap == lsm->levels[lv_off + 1].max_cap){
		if(lv_off + 1 != lsm->max_level){
			lsm_merge(lsm, lv_off + 1);
		}
	}
	uint32_t offset, loc;
	PTR temp = (PTR)malloc(PAGESIZE);
	lsm->compaction = skiplist_init();
	for(int i = 0; i < lsm->levels[lv_off].cur_cap; i++){
		loc = 0;
		offset = lsm->levels[lv_off].array[lsm->levels[lv_off].cur_cap - 1 - i].LPA;
		lseek64(lsm->fd, (off64_t)(PAGESIZE * offset), SEEK_SET);
		read(lsm->fd, temp, PAGESIZE);
		for(int j = 0; j < gepp; j++){ // 221개 전부 차있다고 가정하고 작성
			skiplist_snode_insert(lsm->compaction, (snode*)&temp[loc]);
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
			for(int j = 0; j < gepp; j++){
				skiplist_snode_insert(lsm->compaction, (snode*)&temp[loc]);
				loc += sizeof(uint64_t) * 4 + sizeof(KEYT) + sizeof(ERASET);
			}
			lseek64(lsm->fd, -(off64_t)(PAGESIZE * 2), SEEK_CUR);
		}
		lsm->levels[lv_off + 1].cur_cap = 0;
	}
	free(temp);
	//fwrite 미완
	KEYT key = lsm->compaction->start;
	KEYT end;
	printf("s %ld\n", lsm->compaction->size);
	printf("i %ld\n", lsm->compaction->size/gepp);
	for(int i = 0; i <= lsm->compaction->size/gepp; i++){
		lsm->levels[lv_off + 1].array[i].min = key;
		lsm->levels[lv_off + 1].array[i].memptr = skiplist_lsm_merge(lsm->compaction, key, &key, &end);
		lsm->levels[lv_off + 1].array[i].max = end;
		if(i == lsm->compaction->size/gepp){
			lsm->levels[lv_off + 1].array[i].max = lsm->compaction->end;
		}
		lsm->levels[lv_off + 1].cur_cap++;
		lsm_node_fwrite(lsm, lv_off + 1, i);
	}
	printf("merge end\n");
	//skiplist_dump_key_value(lsm->mixbuf);
	skiplist_free(lsm->compaction);
	return 1;
}

void print_level_status(lsmtree* lsm){
	for(int i = 1; i <= lsm->max_level; i++){
		if(lsm->levels[i].cur_cap > 0){
			printf("level %d's cur_cap is %d\n", i, lsm->levels[i].cur_cap);
		}
	}
}
