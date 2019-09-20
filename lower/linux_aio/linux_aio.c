#define _LARGEFILE64_SOURCE
#include "../../include/settings.h"
#include "../../bench/bench.h"
#include "../../bench/measurement.h"
#include "../../algorithm/Lsmtree/lsmtree.h"
#include "../../include/utils/cond_lock.h"
#include "../../include/utils/thpool.h"
#include "linux_aio.h"
#include <libaio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include<semaphore.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <semaphore.h>

lower_info aio_info={
	.create=aio_create,
	.destroy=aio_destroy,
	.write=aio_push_data,
	.read=aio_pull_data,
	.device_badblock_checker=NULL,
	.trim_block=aio_trim_block,
	.refresh=aio_refresh,
	.stop=aio_stop,
	.lower_alloc=NULL,
	.lower_free=NULL,
	.lower_flying_req_wait=aio_flying_req_wait
};

threadpool thpool;

static int _fd;
static pthread_mutex_t fd_lock,flying_lock,sn_lock,cnt_lock;
static pthread_t t_id;
static io_context_t ctx;
cl_lock *lower_flying;
cl_lock *poller_cl[POLLER_THRD];
bool flying_flag;
//static int write_cnt, read_cnt;
sem_t sem;
bool wait_flag;
bool stopflag;
uint64_t lower_micro_latency;
MeasureTime total_time;
long int a, b, sum1, sum2, max1, max2;

int type_dist[9][1000];
int read_dist[1000], write_dist[1000];

int ptrn_idx;
int rw_pattern[1000];
long int ppa_pattern[1000];

static uint8_t test_type(uint8_t type){
	uint8_t t_type=0xff>>1;
	return type&t_type;
}

int g_sn = 0;
int numio = 0;

#ifdef THPOOL
void io_submit_wrap(void *arg, int id) {
	struct iocb *cb = (struct iocb *)arg;

    pthread_mutex_lock(&sn_lock);
    int sn = g_sn;
    g_sn++;
    if(g_sn%POLLER_THRD == 0)
        g_sn = 0;
    pthread_mutex_unlock(&sn_lock);

	if (io_submit(ctx,1,&cb) !=1) {
        printf("Error on aio_write()\n");
        exit(1);
    }

    pthread_mutex_lock(&cnt_lock);
    numio++;
    pthread_mutex_unlock(&cnt_lock);
    //cl_release(poller_cl[sn]);
}
#endif

void *poller(void *input) {
	algo_req *req;
	int ret;
	struct io_event done_array[AIODEPTH];
	struct io_event *r;
	struct timespec w_t;
	struct iocb *cb;
	w_t.tv_sec=0;
	w_t.tv_nsec=10*1000;
    int sn = (int)input;

    for (int i = 0; ; i++) {
        if (stopflag) {
            pthread_exit(NULL);
        }
        //cl_grap(poller_cl[sn]);
		if((ret=io_getevents(ctx,0,AIODEPTH,done_array,&w_t))){
			for(int i=0; i<ret; i++){
                pthread_mutex_lock(&cnt_lock);
                numio--;
                pthread_mutex_unlock(&cnt_lock);
				r=&done_array[i];
				req=(algo_req*)r->data;
				cb=r->obj;
				if(r->res==-22){
					printf("error! %s %lu %llu\n",strerror(-r->res),r->res2,cb->u.c.offset);
				}else if(r->res!=req->size){
					printf("data size error %d!\n",errno);
				}
				else{
				//	printf("cb->offset:%d cb->nbytes:%d\n",cb->u.c.offset,cb->u.c.nbytes);
				}
				//if(req->parents){
				//}
#ifdef LPRINT
                gettimeofday(&req->lowerend, NULL);
                printf("lower latency [sec, usec] : [%ld, %ld]\n", req->lowerend.tv_sec-req->lowerstart.tv_sec, req->lowerend.tv_usec-req->lowerstart.tv_usec);
#endif
				req->end_req(req);
				cl_release(lower_flying);

				free(r->obj);
			}
		}
		if(lower_flying->now==lower_flying->cnt){
			if(wait_flag){
				wait_flag=false;
				sem_post(&sem);
			}
		}
        if (i == 1-1) i = -1;
    }
	return NULL;
}

uint32_t aio_create(lower_info *li){
	int ret;
	sem_init(&sem,0,0);
	li->NOB=_NOS;
	li->NOP=_NOP;
	li->SOB=BLOCKSIZE * BPS;
	li->SOP=PAGESIZE;
	li->SOK=sizeof(uint32_t);
	li->PPB=_PPB;
	li->PPS=_PPS;
	li->TS=TOTALSIZE;
	li->DEV_SIZE=DEVSIZE;
	li->all_pages_in_dev=DEVSIZE/PAGESIZE;
    memset(li->req_type_cnt, 0, sizeof(uint64_t)*LREQ_TYPE_NUM);

	li->write_op=li->read_op=li->trim_op=0;
	printf("file name : %s\n",LOWER_FILE_NAME);
	//_fd=open(LOWER_FILE_NAME,O_RDWR|O_DIRECT,0644);
#ifdef __cplusplus
	_fd=open(LOWER_FILE_NAME,O_RDWR|O_CREAT|O_DIRECT,0666);
#else
	_fd=open(LOWER_FILE_NAME,O_RDWR|O_CREAT|O_DIRECT,0666);
#endif
	//_fd=open64(LOWER_FILE_NAME,O_RDWR|O_CREAT|O_DIRECT,0666);
	if(_fd==-1){
		printf("file open error!\n");
		exit(1);
	}
	
	//ret=io_setup(QDEPTH,&ctx);
	ret=io_setup(AIODEPTH,&ctx);
	if(ret!=0){
		printf("io setup error\n");
		exit(1);
	}

	//lower_flying=cl_init(QDEPTH,false);
	lower_flying=cl_init(AIODEPTH,false);
    for(int i = 0; i < POLLER_THRD; i++){
        //poller_cl[i]=cl_init(QDEPTH,true);
        poller_cl[i]=cl_init(AIODEPTH,true);
    }

	pthread_mutex_init(&fd_lock,NULL);
	pthread_mutex_init(&aio_info.lower_lock,NULL);
	pthread_mutex_init(&flying_lock,NULL);
	pthread_mutex_init(&sn_lock,NULL);
    pthread_mutex_init(&cnt_lock,NULL);
	sem_init(&sem,0,0);

	measure_init(&li->writeTime);
	measure_init(&li->readTime);
	measure_init(&total_time);
	MS(&total_time);

    stopflag = false;
    for(int i = 0; i < POLLER_THRD; i++)
        pthread_create(&t_id, NULL, &poller, (void*)i);

#ifdef THPOOL
	thpool = thpool_init(NUM_THREAD);
#endif

	return 1;
}

void *aio_refresh(lower_info *li){
	measure_init(&li->writeTime);
	measure_init(&li->readTime);
	li->write_op=li->read_op=li->trim_op=0;
	return NULL;
}
void *aio_destroy(lower_info *li){
	for(int i=0; i<LREQ_TYPE_NUM;i++){
		fprintf(stderr,"%s %lu\n",bench_lower_type(i),li->req_type_cnt[i]);
	}
	close(_fd);

	return NULL;
}
uint64_t offset_hooker(uint64_t origin_offset, uint8_t req_type){
	uint64_t res=origin_offset;
	switch(req_type){
		case TRIM:
			break;
		case MAPPINGR:
			break;
		case MAPPINGW:
			break;
		case GCMR:
			break;
		case GCMW:
			break;
		case DATAR:
			break;
		case DATAW:
			break;
		case GCDR:
			break;
		case GCDW:
			break;
	}
	return res%(aio_info.DEV_SIZE);
}
void *aio_push_data(uint32_t PPA, uint32_t size, value_set* value, bool async,algo_req *const req){
#ifdef NOWRITE
    req->end_req(req);
    return NULL;
#endif
	req->ppa = PPA;
	if(value->dmatag==-1){
		printf("dmatag -1 error!\n");
		exit(1);
	}
	uint8_t t_type=test_type(req->type);
	if(t_type < LREQ_TYPE_NUM){
		aio_info.req_type_cnt[t_type]++;
	}

	struct iocb *cb=(struct iocb*)malloc(sizeof(struct iocb));
	cl_grap(lower_flying);

	//io_prep_pwrite(cb,_fd,(void*)value->value,PAGESIZE,aio_info.SOP*PPA);
    //printf("PPA : %u, size : %u, value : %p, req : %p\n", PPA, size, value, req);
    uint64_t real_offset = offset_hooker((uint64_t)aio_info.SOP*PPA,t_type);
    //printf("write LPA : %u PPA : %u, real_offset : %lu size : %u\n", req->parents->key, PPA, real_offset, size);
	io_prep_pwrite(cb,_fd,(void*)value->value,size,real_offset);
	cb->data=(void*)req;	

#ifdef THPOOL
	while(thpool_num_threads_working(thpool)>=NUM_THREAD);
	thpool_add_work(thpool, io_submit_wrap, (void *)cb);
#ifdef LPRINT
    gettimeofday(&req->lowerstart, NULL);
#endif
#ifdef QPRINT
    //printf("aio queued : %d, req length : %u\n", numio, size);
#endif
#else
	pthread_mutex_lock(&fd_lock);
	if (io_submit(ctx,1,&cb) !=1) {
        printf("Error on aio_write()\n");
        exit(1);
    }
	pthread_mutex_unlock(&fd_lock);
#endif

	return NULL;
}

void *aio_pull_data(uint32_t PPA, uint32_t size, value_set* value, bool async,algo_req *const req){	
#ifdef KPRINT
    printf("aio_pull_data : %d\n", req->parents->key);
#endif
	req->ppa = PPA;
	if(value->dmatag==-1){
		printf("dmatag -1 error!\n");
		exit(1);
	}
	uint8_t t_type=test_type(req->type);
	if(t_type < LREQ_TYPE_NUM){
		aio_info.req_type_cnt[t_type]++;
	}
	
	struct iocb *cb=(struct iocb*)malloc(sizeof(struct iocb));
	cl_grap(lower_flying);
    uint64_t real_offset = offset_hooker((uint64_t)aio_info.SOP*PPA,t_type);
    //printf("read LPA : %u PPA : %u, real_offset : %lu size : %u\n", req->parents->key, PPA, real_offset, size);
	io_prep_pread(cb,_fd,(void*)value->value,size,real_offset);
	cb->data=(void*)req;

#ifdef THPOOL
	while(thpool_num_threads_working(thpool)>=NUM_THREAD);
	thpool_add_work(thpool, io_submit_wrap, (void *)cb);
#ifdef LPRINT
    gettimeofday(&req->lowerstart, NULL);
#endif
#ifdef QPRINT
    //printf("aio queued : %d, req length : %u\n", numio, size);
#endif
#else
	pthread_mutex_lock(&fd_lock);
	if (io_submit(ctx,1,&cb) !=1) {
        printf("Error on aio_write()\n");
        exit(1);
    }
	pthread_mutex_unlock(&fd_lock);
#endif

	return NULL;
}

void *aio_trim_block(uint32_t PPA, bool async){
	aio_info.req_type_cnt[TRIM]++;
	uint64_t range[2];
	//range[0]=PPA*aio_info.SOP;
	range[0]=offset_hooker((uint64_t)PPA*aio_info.SOP,TRIM);
	range[1]=16384*aio_info.SOP;
	ioctl(_fd,BLKDISCARD,&range);
	return NULL;
}

void aio_stop(){}

void aio_flying_req_wait(){
	while (lower_flying->now != 0) {}
	return ;
}
