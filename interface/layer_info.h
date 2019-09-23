#ifndef __H_LINFO__
#define __H_LINFO__
#include "../include/container.h"
#include "threading.h"
#include "interface.h"
#include "buse.h"
#include "../include/settings.h"

//alogrithm layer
extern struct algorithm __normal;
extern struct algorithm __badblock;
extern struct algorithm __demand;
extern struct algorithm algo_pbase;
extern struct algorithm algo_lsm;

//device layer
extern struct lower_info memio_info;
extern struct lower_info aio_info;
extern struct lower_info net_info;
extern struct lower_info my_posix; //posix, posix_memory,posix_async
extern struct lower_info vcu_info;

#if BULK
static uint32_t (*algo_read)(request *const);
static uint32_t (*algo_write)(request *const);
static uint32_t (*algo_remove)(request *const);
static void* (*lower_read)(uint32_t ppa, uint32_t size, value_set *value, bool async, algo_req* const req);
static void* (*lower_write)(uint32_t ppa, uint32_t size, value_set *value, bool async, algo_req* const req);
static void* (*lower_trim)(uint32_t ppa, bool async);
static void* lower_bulk_io(int type, uint32_t ppa, uint32_t size, value_set *value, bool async, algo_req* const req);

static bool algo_bulk_end_req(struct request *const req){
    int32_t* sub_req_cnt = req->sub_req_cnt;
    request* ori_req = req->ori_req;
    uint32_t ori_key = ori_req->key;
    uint32_t i = (req->key)-ori_key;

    //FIXME : if first algo_end_req is not ori_req flush lower buffer
#ifdef KPRINT
    printf("algo_bulk_end_req : %d\n", req->key);
#endif
    (*sub_req_cnt)--;

    if(*sub_req_cnt == 0){
        inf_end_req(req->ori_req);
        free(sub_req_cnt);
        //free(req->ori_req->value);
    }

    return true;
}

static void* lower_bulk_end_req(struct algo_req *const req){
    algo_req** a_reqs = req->sub_algo_req;
    void* (*end_req)(struct algo_req *const) = req->ori_end_req;
    //printf("a_reqs : %p\n", a_reqs);
    for(int i = 0; i < BULK_MAX_REQ; i++){
        if(a_reqs[i] == NULL)
            break;
#ifdef KPRINT
        printf("lower_bulk_end_req : %d\n", a_reqs[i]->parents->key);
#endif
        end_req(a_reqs[i]);
    }
    free(a_reqs);

    return NULL;
}

static uint32_t algo_bulk_io(request *const req, int type){
    int32_t numpage;
    int32_t b_key;
    int32_t* sub_req_cnt;
    request* parse_req;
    value_set* values;

    numpage = (req->bulk_len)/PAGESIZE;
    b_key = req->key;
    parse_req = (request*)malloc(sizeof(request)*numpage);
    values = (value_set*)malloc(sizeof(value_set)*numpage);
    sub_req_cnt = (int*)malloc(sizeof(int));
    *sub_req_cnt = numpage;

    for(int i = 0; i < numpage; i++){
        memcpy(&parse_req[i], req, sizeof(request));
        memcpy(&values[i], req->value, sizeof(value_set));
        values[i].value += i*PAGESIZE;
        parse_req[i].key = b_key+i;
        parse_req[i].value = &values[i];
        parse_req[i].ori_req = &parse_req[0];
        parse_req[i].sub_req_cnt = sub_req_cnt;
        parse_req[i].end_req = algo_bulk_end_req;
        parse_req[i].mark = 0;
        parse_req[i].last = false;
    }
    ((struct buse*)(req->p_req))->value = &values[0];
    free(req->value);
    free(req);
    parse_req[numpage-1].last = true;

    if(type == 0){
        for(int i = 0; i < numpage; i++){
#ifdef KPRINT
            printf("algo_bulk_io : %d\n", parse_req[i].key);
#endif
            algo_read(&parse_req[i]);
        }
    }
    else{
        for(int i = 0; i < numpage; i++){
#ifdef KPRINT
            printf("algo_bulk_io : %d\n", parse_req[i].key);
#endif
            algo_write(&parse_req[i]);
        }
    }

    return 0;
}

static uint32_t algo_bulk_read(request *const req){
    return algo_bulk_io(req, 0);
}

static uint32_t algo_bulk_write(request *const req){
    return algo_bulk_io(req, 1);
}

static uint32_t algo_bulk_remove(request *const req){
  int32_t numpage;
  int32_t b_key;
  int32_t* sub_req_cnt;
  request* parse_req;

  numpage = (req->bulk_len)/PAGESIZE;
  b_key = req->key;
  parse_req = (request*)malloc(sizeof(request)*numpage);
  sub_req_cnt = (int*)malloc(sizeof(int));
  *sub_req_cnt = numpage;

  for(int i = 0; i < numpage; i++){
    memcpy(&parse_req[i], req, sizeof(request));
    parse_req[i].key = b_key+i;
    parse_req[i].value = NULL;
    parse_req[i].ori_req = &parse_req[0];
    parse_req[i].sub_req_cnt = sub_req_cnt;
    parse_req[i].end_req = algo_bulk_end_req;
    parse_req[i].mark = 0;
    parse_req[i].last = false;
  }

  free(req);
  parse_req[numpage-1].last = true;

  for(int i = 0; i < numpage; i++){
    algo_remove(&parse_req[i]);
  }

  return 0;
}

static void do_lower_io(int type, uint32_t ppa, uint32_t cnt, bool async, algo_req** a_reqs){
    uint32_t size = cnt*PAGESIZE;

    a_reqs[0]->sub_algo_req = a_reqs;
    a_reqs[0]->ori_end_req = a_reqs[0]->end_req;
    a_reqs[0]->end_req = lower_bulk_end_req;
    a_reqs[0]->size = size;
    if(type == 0)
        lower_read(ppa, size, a_reqs[0]->parents->value, async, a_reqs[0]);
    else if(type == 1)
        lower_write(ppa, size, a_reqs[0]->parents->value, async, a_reqs[0]);

    return;
}

static void* lower_bulk_io(int type, uint32_t ppa, uint32_t size, value_set *value, bool async, algo_req* const req){
    static uint32_t first_ppa = UINT32_MAX-1;
    static uint32_t prev_ppa = UINT32_MAX-1;
    static uint32_t sub_total = 0;
    static algo_req** a_reqs = NULL;
    uint32_t bsize;
    bool last = false;

    if(req){
        last = req->parents->last;
    }

    if(!size){ //flush algo_reqs
        if(sub_total){
            do_lower_io(type, first_ppa, sub_total, async, a_reqs);
            first_ppa = UINT32_MAX-1;
            prev_ppa = UINT32_MAX-1;
            sub_total = 0;
            a_reqs = NULL;
        }
        if(a_reqs == NULL)
            a_reqs = (algo_req**)calloc(BULK_MAX_REQ, sizeof(algo_req*));

        return NULL;
    }

    if(ppa != prev_ppa+1)
        lower_bulk_io(type, 0, 0, NULL, ASYNC, NULL);

    if(first_ppa == UINT32_MAX-1)
        first_ppa = ppa;
    prev_ppa = ppa;
    a_reqs[sub_total] = req;
    sub_total++;

    if(last)
        lower_bulk_io(type, 0, 0, NULL, ASYNC, NULL);

    return NULL;
}

static void* lower_bulk_read(uint32_t ppa, uint32_t size, value_set *value, bool async, algo_req* const req){
    return lower_bulk_io(0, ppa, size, value, async, req);
}

static void* lower_bulk_write(uint32_t ppa, uint32_t size, value_set *value, bool async, algo_req* const req){
    return lower_bulk_io(1, ppa, size, value, async, req);
}

static void* lower_bulk_trim(uint32_t ppa,bool async){
  return NULL;
}
#endif

static void layer_info_mapping(master_processor *mp){
#if defined(posix) || defined(posix_async) || defined(posix_memory)
	mp->li=&my_posix;
#elif defined(bdbm_drv)
	mp->li=&memio_info;
#elif defined(network)
	mp->li=&net_info;
#elif defined(linux_aio) || defined(linux_aio_test) || defined(linux_aio_buf)
	mp->li=&aio_info;
#elif defined(vcu108)
	mp->li=&vcu_info;
#endif

#ifdef normal
	mp->algo=&__normal;
#elif defined(pftl)
	mp->algo=&algo_pbase;
#elif defined(dftl) || defined(ctoc) || defined(dftl_test) || defined(ctoc_batch) || defined(hash_dftl)
	mp->algo=&__demand;
#elif defined(Lsmtree)
	mp->algo=&algo_lsm;
#elif defined(badblock)
	mp->algo=&__badblock;
#endif

#if BULK
    algo_read = mp->algo->read;
    mp->algo->read = algo_bulk_read;
    algo_write = mp->algo->write;
    mp->algo->write = algo_bulk_write;
    algo_remove = mp->algo->remove;
    mp->algo->remove = algo_bulk_remove;

#ifndef vcu108
    lower_read = mp->li->read;
    mp->li->read = lower_bulk_read;
    lower_write = mp->li->write;
    mp->li->write = lower_bulk_write;
    lower_trim = mp->li->trim_block;
    mp->li->trim_block = lower_bulk_trim;
#endif
#endif
}

#endif
