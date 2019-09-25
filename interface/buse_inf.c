/*
 * busexmp - example memory-based block device using BUSE
 * Copyright (C) 2013 Adam Cozzette
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <argp.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/nbd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

#include "interface.h"
#include "buse.h"
#include "queue.h"
#include "../bench/bench.h"
#include "../bench/measurement.h"
#include "../include/utils/cond_lock.h"
#include "../include/FS.h"
#include "lfqueue/lfqueue.h"

#ifdef pftl
#include "../algorithm/pftl/page.h"
#endif

#ifdef vcu108
#include "../lower/vcu108/vcu108_inf.h"
extern PageTableEntry *pageTable;
extern int blockBase;
#endif

/* BUSE callbacks */

extern master *_master;

#ifdef BUSE_MEASURE
MeasureTime buseTime;
MeasureTime buseendTime;
#endif

struct nbd_reply end_reply;
extern int g_sk;

queue *request_q;
queue *reply_q;
cl_lock *buse_flying;
cl_lock *buse_end_flying;
pthread_mutex_t reply_lock;
pthread_mutex_t rmw_lock;

/* Unlock requests after Read-Modify-Write request is done */
static void do_rmw_unlock(char type){
    if(type == FS_RMW_T)
        pthread_mutex_unlock(&rmw_lock);
}

/* Lock requests following after a Read-Modify-Write(RMW) request
 * 
 * every request are temporarily locked until inflight RMW request is done
 */
static void do_rmw_lock(char type){
    static bool rmw_inflight = false;

    if(type == FS_RMW_T){
        pthread_mutex_lock(&rmw_lock);
        rmw_inflight = true;
    }
    else if(rmw_inflight){
        pthread_mutex_lock(&rmw_lock);
        rmw_inflight = false;
        pthread_mutex_unlock(&rmw_lock);
    }
}

/* Converts logical page address to physical page address
 *
 * obtain card,bus,chip,block,page address by using ftl's page table and vcu's page table
 * calculates physical page address and returns it
 *
 *           ppa format
 *
 * |EMPTY|PAGE|BLOCK|CHIP|BUS|CARD| uint32_t
 * --------------------------------
 * |  5  | 8  | 12  | 3  | 3 | 1  | total 32bit
 */
static uint32_t lpa2ppa(uint32_t lpa){
    uint32_t ftlppa, ppa;
    PageTableEntry vcuppa;

    ftlppa = page_TABLE[lpa].ppa;
    vcuppa = pageTable[ftlppa];
    vcuppa.block = (blockBase+vcuppa.block)%4096; //!!remove effect of blockbase!!
    ppa = (uint32_t)(vcuppa.card |
        vcuppa.bus<<LG_NUM_CARDS |
        vcuppa.chip<<(LG_NUM_BUSES+LG_NUM_CARDS) |
        vcuppa.block<<(LG_CHIPS_PER_BUS+LG_NUM_BUSES+LG_NUM_CARDS) |
        vcuppa.page<<(LG_BLOCKS_PER_CHIP+LG_CHIPS_PER_BUS+LG_NUM_BUSES+LG_NUM_CARDS));

#ifdef DEBUGGETPPA
    fprintf(stderr, "ftlppa : %u\n", ftlppa);
    fprintf(stderr, "(card, bus, chip, block, page) : (%d, %d, %d, %d, %d)\n",
            vcuppa.card, vcuppa.bus, vcuppa.chip, vcuppa.block, vcuppa.page);
    fprintf(stderr, "ppa : %u\n", ppa);
#endif
    
    return ppa;
}

/*
 * Return physical page address(ppa) of file's data
 *
 * calculates lpa from filesystem's blksize, blkmap
 * calculates ppa from FTL's page mapping table, vcu108_inf's mapping table
 */
uint32_t getPhysPageAddr(int fd, size_t byteOffset){
    int blksize;
    uint32_t lba, lpa;

    ioctl(fd, FIGETBSZ, &blksize);
    lba = (int)byteOffset/blksize;
    ioctl(fd, FIBMAP, &lba);

    if(blksize < PAGESIZE)
        lpa = lba/(PAGESIZE/blksize);
    else
        lpa = lba*(blksize/PAGESIZE) + (byteOffset%blksize)/PAGESIZE;

#ifdef DEBUGGETPPA
    fprintf(stderr, "byteOffset : %d\n", byteOffset);
    fprintf(stderr, "blksize : %d\n", blksize);
    fprintf(stderr, "lba : %u\n", lba);
    fprintf(stderr, "lpa : %u\n", lpa);
#endif

    return lpa2ppa(lpa);
}

/* Enqueue replies */
#if (BUSE_ASYNC==1)
static void buse_end_request(uint32_t tmp0, uint32_t tmp2, void* buse_req){
    while(1){
        if(q_enqueue(buse_req, reply_q)){
            cl_release(buse_end_flying);
            break;
        }
    }
    return;
}
#endif

/*
 * Send reply(ack) to nbd
 *
 * cannot send multiple replies simultaneously
 * read : send request header + user data
 * write : send request header
 */
#if (BUSE_ASYNC==1)
void buse_reply(void *args){
#elif (BUSE_ASYNC==0)
void buse_end_request(uint32_t tmp0, uint32_t tmp2, void *args){
#endif
    struct buse *buse_req=(struct buse*)args;
    u_int32_t len=buse_req->len;
    value_set *value=buse_req->value;

#ifdef BUSE_MEASURE
    if(buse_req->type==FS_GET_T)
        MS(&buseendTime);
#endif

#ifdef RPRINT
        printf("reply : %lu, %u\n", buse_req->offset/PAGESIZE, len);
#endif

    memcpy(end_reply.handle, buse_req->handle, 8);
    pthread_mutex_lock(&reply_lock);
    write_all(g_sk,(char*)&end_reply,sizeof(struct nbd_reply));
    if(buse_req->type==FS_GET_T)
        write_all(g_sk,&(value->value)[value->offset],len);
    pthread_mutex_unlock(&reply_lock);
    do_rmw_unlock(buse_req->type);

    if(value)
      inf_free_valueset(value, buse_req->type);
    free(buse_req);

    return;
}

/*
 * Process buse requests with considering request's property
 *
 * caculates a logical page address to support FTL algorithm layer
 * operates differently regards to type of requests
 *      FS_SET_T : determine whether it is normal write request or read-modify-write request
 *                 change type to FS_RMW_T if it is read-modify-write request
 *      FS_GET_T : nothing special
 *      FS_DELETE_T : trim pages by removing it from mapping of FTL
 *                    (FIXME: do not support erase size below PAGESIZE)
 *      FS_RMW_T : send read request first and lock other requests
 *                 when read request is done, modify it and send write request
 *                 after write request is done, unlock other requests
 *
 * Supports various size of requests regardless of PAGESIZE and len except FS_DELETE_T request
 *
 */
static int buse_io(char _type, int sk, void *buf, u_int32_t len, u_int64_t offset, void *handle)
{
    u_int64_t end_offset = offset + len;
    int p_cnt;
    int align;

    struct buse *buse_req = (struct buse*)malloc(sizeof(struct buse));
    buse_req->len=len;
    buse_req->type=_type;
    buse_req->value=NULL;
    buse_req->offset=offset;
    memcpy(buse_req->handle, (char*)handle, 8);

    if(_type==FS_DELETE_T){
      p_cnt = end_offset/PAGESIZE - offset/PAGESIZE;
      align = offset%PAGESIZE==0?0:1;
      p_cnt -= align;
      //FIXME: need small trim logic
      if(p_cnt)
          inf_make_req_buse(buse_req->type, offset/PAGESIZE+align, 0, p_cnt*PAGESIZE, (PTR)buf, (void*)buse_req, buse_end_request);
      else
          buse_end_request(0, 0, buse_req);
      return 0;
    }

    p_cnt = end_offset/PAGESIZE - offset/PAGESIZE;
    p_cnt += end_offset%PAGESIZE==0?0:1;
    align = offset%PAGESIZE + end_offset%PAGESIZE;

    if(_type==FS_SET_T && align>0){
      buse_req->type=FS_RMW_T;
    }

    do_rmw_lock(buse_req->type);
    inf_make_req_buse(buse_req->type, offset/PAGESIZE, offset%PAGESIZE, p_cnt*PAGESIZE, (PTR)buf, (void*)buse_req, buse_end_request);
    
    return 0;
}

/*
 * Queueing buse read requests to request_q
 *
 * other thread will process queued element later
 */
static int buse_make_read(int sk, void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
    struct buse_request *buse_req; 
#ifdef RPRINT
        fprintf(stdout, "R - %lu, %u\n", offset/PAGESIZE, len);
#endif
    buse_req = (struct buse_request*)malloc(sizeof(struct buse_request));
    buse_req->type = FS_GET_T;
    buse_req->buf = buf;
    buse_req->len = len;
    buse_req->offset = offset;
    memcpy(buse_req->handle, userdata, sizeof(buse_req->handle));

    while(1){
        if(q_enqueue((void*)buse_req, request_q)){
            cl_release(buse_flying);
            break;
        }
    }
    return 0;
}

/*
 * Queueing buse write requests to request_q
 *
 * other thread will process queued element later
 */
static int buse_make_write(int sk, void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
    struct buse_request *buse_req; 
#ifdef RPRINT
        fprintf(stdout, "W - %lu, %u\n", offset/PAGESIZE, len);
#endif
    buse_req = (struct buse_request*)malloc(sizeof(struct buse_request));
    buse_req->type = FS_SET_T;
    buse_req->buf = buf;
    buse_req->len = len;
    buse_req->offset = offset;
    memcpy(buse_req->handle, userdata, sizeof(buse_req->handle));
    while(1){
        if(q_enqueue((void*)buse_req, request_q)){
            cl_release(buse_flying);
            break;
        }
    }

    return 0;
}

/*
 * Queueing buse trim requests to request_q
 * 
 * other thread will process queued element later
 */
static int buse_make_trim(int sk, u_int64_t from, u_int32_t len, void *userdata)
{
    struct buse_request *buse_req; 
#ifdef RPRINT
    fprintf(stdout, "T - %lu, %u\n", from/PAGESIZE, len);
#endif
    buse_req = (struct buse_request*)malloc(sizeof(struct buse_request));
    buse_req->type = FS_DELETE_T;
    buse_req->buf = NULL;
    buse_req->len = len;
    buse_req->offset = from;
    memcpy(buse_req->handle, userdata, sizeof(buse_req->handle));
    while(1){
        if(q_enqueue((void*)buse_req, request_q)){
            cl_release(buse_flying);
            break;
        }
    }
    return 0;
}

static int buse_read(int sk, void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
#ifdef RPRINT
    fprintf(stdout, "R - %lu, %u\n", offset/PAGESIZE, len);
#endif
    
    buse_io(FS_GET_T,sk,buf,len,offset,userdata);

    return 0;
}

static int buse_write(int sk, void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
#ifdef RPRINT
    fprintf(stdout, "W - %lu, %u\n", offset/PAGESIZE, len);
#endif

    buse_io(FS_SET_T,sk,buf,len,offset,userdata);

    return 0;
}

static void buse_disc(int sk, void *userdata)
{
#ifdef RPRINT
    fprintf(stdout, "Received a disconnect request.\n");
#endif
}

static int buse_flush(int sk, void *userdata)
{
#ifdef RPRINT
    fprintf(stdout, "Received a flush request.\n");
#endif
    return 0;
}

static int buse_trim(int sk, u_int64_t from, u_int32_t len, void *userdata)
{
#ifdef RPRINT
    fprintf(stdout, "T - %lu, %u\n", from/PAGESIZE, len);
#endif

    buse_io(FS_DELETE_T,sk,NULL,len,from,userdata);

    return 0;
}



#if (BUSE_ASYNC==1)

/*
 * Dequeue requests and run buse_io
 */
void* buse_request_main(void* args){
    struct buse_request *buse_req;
#ifdef LPRINT
    struct timeval infstart, infend;
#endif
    while(1){
		cl_grap(buse_flying);
        buse_req = (struct buse_request*)q_dequeue(request_q);
        if(!buse_req)
            continue;
#ifdef LPRINT
        gettimeofday(&infstart, NULL);
#endif
        buse_io(buse_req->type, g_sk, buse_req->buf, buse_req->len, buse_req->offset, buse_req->handle);
#ifdef LPRINT
        gettimeofday(&infend, NULL);
        printf("inf latency [sec, usec] : [%ld, %ld]\n", infend.tv_sec-infstart.tv_sec, infend.tv_usec-infstart.tv_usec);
#endif
        free(buse_req);
    }
    return NULL;
}

/*
 * Dequeue replies and run buse_reply
 */
void* buse_reply_main(void* args){
    void *buse_req;
    while(1){
		cl_grap(buse_end_flying);
        buse_req = q_dequeue(reply_q);
        if(!buse_req)
            continue;
        buse_reply(buse_req);
    }
    return NULL;
}
#endif

/*
 * Execute command and wait until it ends
 */
int run_exec(char* cmd){
    char s[100];
    char* args[20];
    char* token;
    pid_t pid;
    int status;

    printf("\nInput cmd : %s\n", cmd);

    pid = fork();
    if(!pid){
        memcpy(s, cmd, strlen(cmd)+1);
        token = strtok(s, " ");
        args[0] = token;
        int i;
        for(i = 1; token != NULL; i++){
            token = strtok(NULL, " ");
            args[i] = token;
        }
        args[i] = NULL;
        printf("*****%s running!*****\n", args[0]);
        execvp(args[0], args);
    }

    waitpid(pid, &status, 0);
    if(!WIFEXITED(status)){
        fprintf(stderr, "child exited with error\n");
        abort();
    }

    printf("******terminated!******\n\n");

    return pid;
}

struct arguments {
    unsigned long long size;
    char * device;
    int verbose;
};

static struct arguments arguments;
static struct buse_operations aop;
static pthread_t request_tid[NUM_BUSE_REQ_MAIN];
static pthread_t reply_tid;
static pthread_t bmain_tid;


/* Wrapping function for pthread create */
void* buse_main(void* args){
    return (void*)__buse_main(arguments.device, &aop, (void *)&arguments.verbose);
}

/*
 * Initialize buse, interface, ftl, vcu108 layer
 * 
 * setting device with buse interface and devcie size
 * create buse request and reply threads (reply thread must be single thread)
 */
int buse_init() {
    arguments.verbose = 0;
    arguments.device = DEVNAME;

#if (BUSE_ASYNC==1)
    aop.read = buse_make_read;
    aop.write = buse_make_write;
    aop.trim = buse_make_trim;
#elif (BUSE_ASYNC==0)
    aop.read = buse_read;
    aop.write = buse_write;
    aop.trim = buse_trim;
#endif
    aop.disc = buse_disc;
    aop.flush = buse_flush;
    aop.size = TOTALSIZE;
    aop.blksize = 0;
    aop.size_blocks = 0;

    inf_init(0,0);
#ifdef BUSE_MEASURE
    measure_init(&buseTime);
    measure_init(&buseendTime);
#endif
    pthread_mutex_init(&reply_lock, NULL);
    pthread_mutex_init(&rmw_lock, NULL);
    end_reply.magic = htonl(NBD_REPLY_MAGIC);
    end_reply.error = htonl(0);
#if (BUSE_ASYNC==1)
    q_init(&request_q, QSIZE);
    q_init(&reply_q, QSIZE);
    buse_flying=cl_init(QDEPTH*2,true);
    buse_end_flying=cl_init(QDEPTH*2,true);
    for(int i = 0; i < NUM_BUSE_REQ_MAIN; i++){
        pthread_create(&request_tid[i], NULL, buse_request_main, NULL);
    }
    pthread_create(&reply_tid, NULL, buse_reply_main, NULL);
#endif

    pthread_create(&bmain_tid, NULL, buse_main, NULL);
    sleep(5); //wait to make sure buse_main initialzing is done
    //buse_main(arguments.device, &aop, (void *)&arguments.verbose);
    return 0;
}

/*
 * Wait until SIGINT or SIGTERM received 
 */
void buse_wait(){
    void* ret;

    pthread_join(bmain_tid, &ret);
    if((int64_t)ret != EXIT_SUCCESS){
        fprintf(stderr, "buse main exit failure!\n");
        abort();
    }
}

/* 
 * Free buse, interface, ftl, vcu108 layer
 *
 * cancel buse request and reply threads
 * free variables
 */
int buse_free() {
#if (BUSE_ASYNC==1)
    for(int i = 0; i < NUM_BUSE_REQ_MAIN; i++){
        pthread_cancel(request_tid[i]);
    }
    pthread_cancel(reply_tid);
    disconnect_nbd(0);
    q_free(request_q);
    q_free(reply_q);
#endif
    inf_free();

#ifdef BUSE_MEASURE
    printf("buseTime : ");
    measure_adding_print(&buseTime);
    printf("buseendTime : ");
    measure_adding_print(&buseendTime);
#endif
    return 0;
}
