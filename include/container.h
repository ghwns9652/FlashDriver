#ifndef __H_CONTAINER__
#define __H_CONTAINER__
 
#include "settings.h"
#include "types.h"
#include "utils.h"
#include "sem_lock.h"
#include <stdarg.h>
#include <pthread.h>

typedef struct lower_info lower_info;
typedef struct algorithm algorithm;
typedef struct algo_req algo_req;
typedef struct request request;

typedef struct upper_request{
	const FSTYPE type;
	const KEYT key;
	uint32_t length;
	V_PTR value;
	//anything
}upper_request;

typedef struct value_set{
	PTR value;
	uint32_t length;
  request* req;
	int dmatag; //-1 == not dma_alloc, others== dma_alloc
	uint32_t ppa;
	bool from_app;
	PTR rmw_value;
	uint8_t status;
	uint32_t rmw_len;
	uint32_t offset;
}value_set;

struct request {
	FSTYPE type;
	KEYT key;
	uint64_t ppa;/*it can be the iter_idx*/
	uint32_t seq;
	volatile int num; /*length of requests*/
	volatile int cpl; /*number of completed requests*/
	int not_found_cnt;
	value_set *value;
	value_set **multi_value;
	char **app_result;

	KEYT *multi_key;
	bool (*end_req)(struct request *const);
	void *(*special_func)(void *);
	bool (*added_end_req)(struct request *const);
	bool isAsync;
	void *p_req;
	void (*p_end_req)(uint32_t,uint32_t,void*);
	void *params;
	void *__hash_node;
	//pthread_mutex_t async_mutex;
	fdriver_lock_t sync_lock;
	int mark;

/*s:for application req*/
	char *target_buf;
	uint32_t inter_offset;
	uint32_t target_len;
	char istophalf;
	FSTYPE org_type;
/*e:for application req*/

	uint8_t type_ftl;
	uint8_t type_lower;
	uint8_t before_type_lower;
	bool isstart;
	MeasureTime latency_checker;

/*for bulk IO */
  KEYT bulk_len; 
    request* ori_req;
    int32_t* sub_req_cnt;
    bool last;

#ifdef hash_dftl
	void *hash_params;
	struct request *parents;
#endif
};

struct algo_req{
	uint64_t ppa;
    uint32_t size;
	request * parents;
	MeasureTime latency_lower;
	uint8_t type;
	bool rapid;
	uint8_t type_lower;
	//0: normal, 1 : no tag, 2: read delay 4:write delay
	void *(*end_req)(struct algo_req *const);
	void *(*ori_end_req)(struct algo_req *const);
	void *params;
    /*for bulk IO*/
    algo_req** sub_algo_req;
#ifdef LPRINT
    struct timeval lowerstart;
    struct timeval lowerend;
#endif
};

struct lower_info {
	uint32_t (*create)(struct lower_info*);
	void* (*destroy)(struct lower_info*);
	void* (*write)(uint32_t ppa, uint32_t size, value_set *value,bool async,algo_req * const req);
	void* (*read)(uint32_t ppa, uint32_t size, value_set *value,bool async,algo_req * const req);
	void* (*device_badblock_checker)(uint32_t ppa,uint32_t size,void *(*process)(uint64_t, uint8_t));
	void* (*trim_block)(uint32_t ppa,bool async);
	void* (*refresh)(struct lower_info*);
	void (*stop)();
	int (*lower_alloc) (int type, char** buf);
	void (*lower_free) (int type, int dmaTag);
	void (*lower_flying_req_wait) ();
	void (*lower_show_info)();

	lower_status (*statusOfblock)(BLOCKT);
	pthread_mutex_t lower_lock;
	
	uint64_t write_op;
	uint64_t read_op;
	uint64_t trim_op;
	
	MeasureTime writeTime;
	MeasureTime readTime;

	uint32_t NOB;
	uint32_t NOP;
	uint32_t SOK;
	uint32_t SOB;
	uint32_t SOP;
	uint32_t PPB;
	uint32_t PPS;
	uint64_t TS;
	uint64_t DEV_SIZE;//for sacle up test
	uint64_t all_pages_in_dev;//for scale up test

	uint64_t req_type_cnt[LREQ_TYPE_NUM];
	//anything
};

struct algorithm{
	/*interface*/
	uint32_t (*create) (lower_info*,struct algorithm *);
	void (*destroy) (lower_info*, struct algorithm *);
	uint32_t (*read)(request *const);
	uint32_t (*write)(request *const);
	uint32_t (*remove)(request *const);
	uint32_t (*iter_create)(request *const);
	uint32_t (*iter_next)(request *const);
	uint32_t (*iter_next_with_value)(request *const);
	uint32_t (*iter_release)(request *const);
	uint32_t (*iter_all_key)(request *const);
	uint32_t (*iter_all_value)(request *const);
	uint32_t (*multi_set)(request *const,int num);
	uint32_t (*multi_get)(request *const,int num);
	uint32_t (*range_query)(request *const);
	lower_info* li;
	void *algo_body;
};
#endif
