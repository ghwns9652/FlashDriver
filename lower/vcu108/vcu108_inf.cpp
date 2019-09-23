#include <sys/time.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "dmaManager.h"
#include "FlashIndication.h"
#include "FlashRequest.h"

#include "../../include/settings.h"
#include "../../bench/bench.h"
#include "../../bench/measurement.h"
#include "../../algorithm/Lsmtree/lsmtree.h"
#include "../../include/utils/cond_lock.h"
#include "../../include/utils/thpool.h"
#include "../../interface/queue.h"
#include "vcu108_inf.h"

#if defined(SIMULATION)
#define PAGES_PER_BLOCK 1 
#define BLOCKS_PER_CHIP 1 
#define CHIPS_PER_BUS 8
#define NUM_BUSES 8
#define NUM_CARDS 2

#else
#define PAGES_PER_BLOCK 256
#define BLOCKS_PER_CHIP 4096
#define CHIPS_PER_BUS 8
#define NUM_BUSES 8
#define NUM_CARDS 2
#endif

#define PAGE_SIZE (8192*2)
#define PAGE_SIZE_VALID (8224)
#define NUM_TAGS 128

//#define DEBUG 1
//#define TAGPRINT
#ifdef TAGPRINT
int busy_cnt = 0;
pthread_mutex_t busy_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

#define NUM_VCU_THREAD 8
pthread_t vcu_tid;

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

FlashRequestProxy *device;

pthread_mutex_t flashReqMutex;
pthread_cond_t flashFreeTagCond;

pthread_mutex_t writeReqMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readReqMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t eraseReqMutex = PTHREAD_MUTEX_INITIALIZER;

//8k * 128p
size_t dstAlloc_sz = PAGE_SIZE * NUM_TAGS *sizeof(unsigned char);
size_t srcAlloc_sz = PAGE_SIZE * NUM_TAGS *sizeof(unsigned char);
int dstAlloc;
int srcAlloc;
unsigned int ref_dstAlloc; 
unsigned int ref_srcAlloc; 
unsigned int* dstBuffer;
unsigned int* srcBuffer;
unsigned int* readBuffers[NUM_TAGS];
unsigned int* writeBuffers[NUM_TAGS];
TagTableEntry readTagTable[NUM_TAGS]; 
TagTableEntry writeTagTable[NUM_TAGS]; 
TagTableEntry eraseTagTable[NUM_TAGS]; 
FlashStatusT flashStatus[NUM_CARDS][NUM_BUSES][CHIPS_PER_BUS][BLOCKS_PER_CHIP];
PageTableEntry *pageTable;

int testPass = 1;
bool verbose = true;
int curReadsInFlight = 0;
int curWritesInFlight = 0;
int curErasesInFlight = 0;

int blockBase = 0; // so that we are not erasing the same block;
int badBlockCnt = 0;

unsigned int hashAddrToData(int card, int bus, int chip, int blk, int word) {
    return ((card<<28) + (bus<<24) + (chip<<20) + (blk<<16) + word);
}

void lowerPageAlloc(uint32_t ppa){
    static uint32_t card, bus, chip, block, page = 0;

    pageTable[ppa].card = card;
    pageTable[ppa].bus = bus;
    pageTable[ppa].chip = chip;
    pageTable[ppa].block = block;
    pageTable[ppa].page = page; 
    DEBUG_PRINT(stderr, "Allocated! (card, bus, chip, block, page) = (%d, %d, %d, %d, %d)\n", card, bus, chip, block, page);

    do{
        if(++card != NUM_CARDS) continue;
        else card = 0;

        if(++bus != NUM_BUSES) continue;
        else bus = 0;

        if(++chip != CHIPS_PER_BUS) continue;
        else chip = 0;

        if(++block != BLOCKS_PER_CHIP) continue;
        else block = 0;

        if(++page != PAGES_PER_BLOCK) continue;
        else page = 0;

    }while(flashStatus[card][bus][chip][block] != ERASED);

    return;
}

lower_info vcu_info={
	.create=vcu_create,
	.destroy=vcu_destroy,
	.write=vcu_push_data,
	.read=vcu_pull_data,
	.device_badblock_checker=NULL,
	.trim_block=vcu_trim_block,
	.refresh=vcu_refresh,
	.stop=vcu_stop,
	.lower_alloc=NULL,
	.lower_free=NULL,
	.lower_flying_req_wait=vcu_flying_req_wait
};

class FlashIndication : public FlashIndicationWrapper{
public:
  virtual void readDone(unsigned int tag, uint64_t cycles) {
    fprintf(stderr, "ERROR: pagedone: no indication should be really sent");
  }

  virtual void writeDone(unsigned int tag, uint64_t cycles) {
    DEBUG_PRINT("LOG: writedone, tag=%d, FPGA cycles = %lu\n", tag, cycles);

    //TODO probably should use a diff lock
    /*
    curWritesInFlight--;
    if ( curWritesInFlight < 0 ) {
      fprintf(stderr,"LOG: **ERROR: Write requests in flight cannot be negative %d\n", curWritesInFlight );
      curWritesInFlight = 0;
    }
    */
    if ( writeTagTable[tag].busy == false ) {
      fprintf(stderr, "LOG: **ERROR: received unused buffer Write done %d\n", tag);
      testPass = 0;
    }
        
    int card = writeTagTable[tag].card;
    int bus = writeTagTable[tag].bus;
    int chip = writeTagTable[tag].chip;
    int block = writeTagTable[tag].block;
    int page = writeTagTable[tag].page;
    DEBUG_PRINT("LOG: writedone, (card,bus,chip,block,page) = (%d,%d,%d,%d,%d)\n", card, bus,chip,block,page);
    
    pthread_mutex_lock(&flashReqMutex);
    if ( flashStatus[card][bus][chip][block] == WRITTEN ) {
      fprintf(stderr, "LOG:: **ERROR (card,bus,chip,block,page) = (%d,%d,%d,%d,%d) has been written before\n", card, bus,chip,block,page);
    }
    if((page+1) == PAGES_PER_BLOCK)
        flashStatus[card][bus][chip][block] = WRITTEN;
    pthread_mutex_unlock(&flashReqMutex);
    
    algo_req* req = writeTagTable[tag].req;
    writeTagTable[tag].busy = false;
    if(req){
        req->end_req(req);
    }
  }

  virtual void eraseDone(unsigned int tag, unsigned int status, uint64_t cycles) {
    DEBUG_PRINT("LOG: eraseDone, tag=%d, status=%d, FPGA cycles = %lu\n", tag, status, cycles);
    //pthread_mutex_lock(&eraseReqMutex);
    
    int card = eraseTagTable[tag].card;
    int bus = eraseTagTable[tag].bus;
    int chip = eraseTagTable[tag].chip;
    int block = eraseTagTable[tag].block;

    pthread_mutex_lock(&flashReqMutex);
    if (status != 0) {
      printf("LOG: detected bad block with tag = %d\n", tag);
      flashStatus[card][bus][chip][block] = ERASED_BAD;
    } else {
      flashStatus[card][bus][chip][block] = ERASED;
    }
    pthread_mutex_unlock(&flashReqMutex);

    /*
    if ( curErasesInFlight < 0 ) {
      DEBUG_PRINT( "LOG: **ERROR: erase requests in flight cannot be negative %d\n", curErasesInFlight );
      curErasesInFlight = 0;
    }
    */
    if ( eraseTagTable[tag].busy == false ) {
      DEBUG_PRINT( "LOG: **ERROR: received unused tag erase done %d\n", tag);
      testPass = 0;
    }
    eraseTagTable[tag].busy = false;
    //curErasesInFlight--;
    //pthread_mutex_unlock(&eraseReqMutex);
  }

  virtual void debugDumpResp (unsigned int card, unsigned int debug0, unsigned int debug1,  unsigned int debug2, unsigned int debug3, unsigned int debug4, unsigned int debug5) {
    DEBUG_PRINT( "LOG: DEBUG DUMP: card = %d, gearSend = %d, gearRec = %d, aurSend = %d, aurRec = %d, readSend=%d, writeSend=%d\n",card, debug0, debug1, debug2, debug3, debug4, debug5);
  }

  FlashIndication(unsigned int id) : FlashIndicationWrapper(id){}
};

FlashIndication *testIndication;

int getNumReadsInFlight() { return curReadsInFlight; }
int getNumWritesInFlight() { return curWritesInFlight; }
int getNumErasesInFlight() { return curErasesInFlight; }



//TODO: more efficient locking
int waitIdleEraseTag() {
  int tag = -1;
  while ( tag < 0 ) {
	pthread_mutex_lock(&eraseReqMutex);

    for ( int t = 0; t < NUM_TAGS; t++ ) {
      if ( !eraseTagTable[t].busy ) {
        eraseTagTable[t].busy = true;
        tag = t;
        break;
      }
    }
	pthread_mutex_unlock(&eraseReqMutex);
  }
  return tag;
}


//TODO: more efficient locking
int waitIdleWriteBuffer() {
  int tag = -1;
  while ( tag < 0 ) {
	pthread_mutex_lock(&writeReqMutex);

    for ( int t = 0; t < NUM_TAGS; t++ ) {
      if ( !writeTagTable[t].busy) {
        writeTagTable[t].busy = true;
        tag = t;
        break;
      }
    }
	pthread_mutex_unlock(&writeReqMutex);
  }
  return tag;
}



//TODO: more efficient locking
int waitIdleReadBuffer() {
  int tag = -1;
  while ( tag < 0 ) {
	pthread_mutex_lock(&readReqMutex);

    for ( int t = 0; t < NUM_TAGS; t++ ) {
      if ( !readTagTable[t].busy ) {
        readTagTable[t].busy = true;
        tag = t;
        break;
      }
    }
	pthread_mutex_unlock(&readReqMutex);
  }
  return tag;
}

int waitInflight(uint32_t ppa) {
  while (1) {
	pthread_mutex_lock(&flashReqMutex);
    if(!(pageTable[ppa].inflight))
        break;
	pthread_mutex_unlock(&flashReqMutex);
  }
  pthread_mutex_unlock(&flashReqMutex);

  return 0;
}

void eraseBlock(int card, int bus, int chip, int block, int tag) {
  DEBUG_PRINT( "LOG: sending erase block request with tag=%d @%d %d %d %d 0\n", tag, card, bus, chip, block );
  device->eraseBlock(card,bus,chip,(blockBase+block)%4096,tag);
}


void writePage(int card, int bus, int chip, int block, int page, int tag) {
    DEBUG_PRINT( "LOG: sending write page request with tag=%d @%d %d %d %d %d\n", tag, card, bus, chip, block, page );
    device->writePage(card,bus,chip,(blockBase+block)%4096,page,tag);
}

void readPage(int card, int bus, int chip, int block, int page, int tag) {
  DEBUG_PRINT( "LOG: sending read page request with tag=%d @%d %d %d %d %d\n", tag, card, bus, chip, block, page );
  device->readPage(card,bus,chip,(blockBase+block)%4096,page,tag);
}


void *check_read_buffer_done(void *ptr){

  int tag = 0;
  int flag_word_offset=PAGE_SIZE_VALID/sizeof(unsigned int);
  while ( true ){
    // DEBUG_PRINT("LOG: readBuffers[%d][%d]=%x\n", tag, flag_word_offset, readBuffers[tag][flag_word_offset] );
    if ( readBuffers[tag][flag_word_offset] == (unsigned int)-1 ) {

      DEBUG_PRINT("LOG: pagedone: tag=%d; inflight=%d\n", tag, curReadsInFlight );
      
      //pthread_mutex_lock(&readReqMutex);
      /*
      curReadsInFlight --;
      if ( curReadsInFlight < 0 ) {
        DEBUG_PRINT( "LOG: **ERROR: Read requests in flight cannot be negative %d\n", curReadsInFlight );
        curReadsInFlight = 0;
      }
      */
      if ( readTagTable[tag].busy == false ) {
        DEBUG_PRINT( "LOG: **ERROR: received unused buffer read done %d\n", tag);
        testPass = 0;
      }
      algo_req* req = readTagTable[tag].req;
      if(req){
          value_set* value = req->parents->value;
          memcpy(value->value, readBuffers[tag], PAGESIZE);
          req->end_req(req);
      }
      readTagTable[tag].busy = false;
#ifdef TAGPRINT
      pthread_mutex_lock(&busy_lock);
      busy_cnt--;
      pthread_mutex_unlock(&busy_lock);
#endif
      readBuffers[tag][flag_word_offset] = 0;
      //pthread_mutex_unlock(&readReqMutex);
    }

    tag = (tag + 1)%NUM_TAGS;
  }
    
}

queue* vcu_q;
static cl_lock* vcu_flying;

/* VCU108 run as async mode */
static void* vcu_request_main(void* args){
    struct vcu_request *vcu_req;

    while(1){
		cl_grap(vcu_flying);
        vcu_req = (struct vcu_request*)q_dequeue(vcu_q);
        if(!vcu_req)
            continue;
        switch(vcu_req->type){
            case FS_SET_T:
                writePage(vcu_req->card, vcu_req->bus, vcu_req->chip, vcu_req->block, vcu_req->page, vcu_req->tag);
                break;
            case FS_GET_T:
                readPage(vcu_req->card, vcu_req->bus, vcu_req->chip, vcu_req->block, vcu_req->page, vcu_req->tag);
                break;
            case FS_DELETE_T:
                eraseBlock(vcu_req->card, vcu_req->bus, vcu_req->chip, vcu_req->block, vcu_req->tag);
                break;
        }

        free(vcu_req);
    }

    return NULL;
}

uint32_t vcu_create(lower_info *li){
  DEBUG_PRINT( "Main Start\n");
  device = new FlashRequestProxy(IfcNames_FlashRequestS2H); //connectal initialization
  testIndication = new FlashIndication(IfcNames_FlashIndicationH2S); //connectal callback initialization

  DmaManager *dma = platformInit(); //MMU, dma, etc initialization

  DEBUG_PRINT( "Main::allocating memory...\n");

  srcAlloc = portalAlloc(srcAlloc_sz, 0); //src fd allocation
  dstAlloc = portalAlloc(dstAlloc_sz, 0); //dst fd allocation

  srcBuffer = (unsigned int *)portalMmap(srcAlloc, srcAlloc_sz); //mmap
  dstBuffer = (unsigned int *)portalMmap(dstAlloc, dstAlloc_sz);

  portalCacheFlush(srcAlloc, srcBuffer, srcAlloc_sz, 1);
  portalCacheFlush(dstAlloc, dstBuffer, dstAlloc_sz, 1);
  DEBUG_PRINT( "Main::flush and invalidate complete\n");

  pthread_mutex_init(&flashReqMutex, NULL);
  pthread_cond_init(&flashFreeTagCond, NULL);

  printf( "Done initializing hw interfaces\n" ); fflush(stdout);


  ref_dstAlloc = dma->reference(dstAlloc);
  ref_srcAlloc = dma->reference(srcAlloc);

  fprintf(stderr, "dstAlloc = %x\n", dstAlloc); 
  fprintf(stderr, "srcAlloc = %x\n", srcAlloc); 
	
  device->setDmaReadRef(ref_srcAlloc);
  device->setDmaWriteRef(ref_dstAlloc);

  pageTable = (PageTableEntry*)malloc(sizeof(PageTableEntry)*NUM_CARDS*NUM_BUSES*CHIPS_PER_BUS*BLOCKS_PER_CHIP*PAGES_PER_BLOCK);
  memset(pageTable, 0, sizeof(PageTableEntry)*NUM_CARDS*NUM_BUSES*CHIPS_PER_BUS*BLOCKS_PER_CHIP*PAGES_PER_BLOCK);

  q_init(&vcu_q, QSIZE);
  vcu_flying=cl_init(QDEPTH*2,true);

  pthread_create(&vcu_tid, NULL, vcu_request_main, NULL);

  for (int t = 0; t < NUM_TAGS; t++) { //allocate buffer address for each tag
    readTagTable[t].busy = false;
    writeTagTable[t].busy = false;
    int byteOffset = t * PAGE_SIZE;
    // device->addDmaWriteRefs(ref_dstAlloc, byteOffset, t);
    // device->addDmaReadRefs(ref_srcAlloc, byteOffset, t);
    readBuffers[t] = dstBuffer + byteOffset/sizeof(unsigned int);
    writeBuffers[t] = srcBuffer + byteOffset/sizeof(unsigned int);
  }
	
  for (int blk=0; blk<BLOCKS_PER_CHIP; blk++) {
    for (int c=0; c<CHIPS_PER_BUS; c++) {
      for (int bus=0; bus< CHIPS_PER_BUS; bus++) {
        for (int card=0; card < NUM_CARDS; card++) {
          // fprintf(stderr,"flashStatus[%d][%d][%d][%d] = UNINIT\n",card,bus,c,blk);
          flashStatus[card][bus][c][blk] = UNINIT;
        }
      }
    }
  }

  fprintf(stderr, "Done initializing flashStatus\n");

  for (int t = 0; t < NUM_TAGS; t++) {
    for ( unsigned int i = 0; i < PAGE_SIZE/sizeof(unsigned int); i++ ) {
      readBuffers[t][i] = 0;
      writeBuffers[t][i] = 0;
    }
  }

  for (int t = 0; t < NUM_TAGS; t++) {
      for (unsigned int w=0; w<PAGE_SIZE/sizeof(unsigned int); w++) {
          writeBuffers[t][w] = hashAddrToData(0, 0, 0, 0, w); 
      }
  }

  fprintf(stderr, "Done initializing buffers\n");
  /* this variable is our reference to the second thread */
  pthread_t check_thread;

  /* create a second thread which executes inc_x(&x) */
  if(pthread_create(&check_thread, NULL, check_read_buffer_done, NULL)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  fprintf(stderr, "Done Spinning check read done thread\n");

#if not defined(SIMULATION)
  srand(time(NULL));
  blockBase=rand()%4096;
#endif

  fprintf(stderr, "Done initializing blockBase = %u\n", blockBase);
  
  fprintf(stderr, "FPGA starts\n");
  device->start(0);
  fprintf(stderr, "LOG: Starting Correctness TEST...\n");
  timespec start, now;
  clock_gettime(CLOCK_REALTIME, & start);
  
  int tag;
  for (int blk = 0; blk < BLOCKS_PER_CHIP; blk++){
    for (int chip = 0; chip < CHIPS_PER_BUS; chip++){
      for (int bus = 0; bus < NUM_BUSES; bus++){
        for (int card = 0; card < NUM_CARDS; card++){
            tag = waitIdleEraseTag();

            eraseTagTable[tag].card = card;
            eraseTagTable[tag].bus = bus;
            eraseTagTable[tag].chip = chip;
            eraseTagTable[tag].block = blk;

            eraseBlock(card, bus, chip, blk, tag); 
        }
      }
    }
  }

  
  while (true) {
    usleep(100);
    if ( getNumErasesInFlight() == 0 ) break;
  }
  clock_gettime(CLOCK_REALTIME, & now);
  fprintf(stderr, "LOG: finished erasing to flash!\n");

  for(int block = 0; block < BLOCKS_PER_CHIP; block++){
      for(int chip = 0; chip < CHIPS_PER_BUS; chip++){
          for(int bus = 0; bus < NUM_BUSES; bus++){
              for(int card = 0; card < NUM_CARDS; card++){
                  if(flashStatus[card][bus][chip][block]==ERASED_BAD){
                      fprintf(stderr, "BAD BLOCK : (%d %d %d %d)\n", card, bus, chip, block);
                      badBlockCnt++;
                  }
              }
          }
      }
  }

  fprintf(stderr, "LOG: Bad Block Detection Done...\n");

  return 0;
}

void *vcu_refresh(lower_info *li){
  //need?
	measure_init(&li->writeTime);
	measure_init(&li->readTime);
	li->write_op=li->read_op=li->trim_op=0;
	return NULL;
}

void *vcu_destroy(lower_info *li){
  //need check
  return NULL;
}

static void *enqRequest(FSTYPE type, int card, int bus, int chip, int block, int page, int tag){
    struct vcu_request *vcu_req;

    vcu_req = (struct vcu_request*)malloc(sizeof(struct vcu_request));
    vcu_req->type = type;
    vcu_req->card = card;
    vcu_req->bus = bus;
    vcu_req->chip = chip;
    vcu_req->block = block;
    vcu_req->page = page;
    vcu_req->tag = tag;

    while(1){
        if(q_enqueue((void*)vcu_req, vcu_q)){
            cl_release(vcu_flying);
            break;
        }
    }
    return 0;
}

void *vcu_push_data(uint32_t PPA, uint32_t size, value_set* value, bool async,algo_req *const req){

  lowerPageAlloc(PPA);
  PageTableEntry pte = pageTable[PPA];

  if(flashStatus[pte.card][pte.bus][pte.chip][pte.block] == ERASED_BAD){
      fprintf(stderr, "Error: push data bad block\n");
      fprintf(stderr, "card bus chip block page: %d %d %d %d %d\n", pte.card, pte.bus, pte.chip, pte.block, pte.page);
      fprintf(stderr, "PPA: %u\n", PPA);
      abort();
  }

  int tag = waitIdleWriteBuffer();
  writeTagTable[tag].req = req;

  memcpy(writeBuffers[tag], value->value, PAGESIZE);
  pageTable[PPA].inflight = true;

  writeTagTable[tag].card = pte.card;
  writeTagTable[tag].bus = pte.bus;
  writeTagTable[tag].chip = pte.chip;
  writeTagTable[tag].block = pte.block;
  writeTagTable[tag].page = pte.page;

  enqRequest(FS_SET_T, pte.card, pte.bus, pte.chip, pte.block, pte.page, tag);
  return NULL;
}

void *vcu_pull_data(uint32_t PPA, uint32_t size, value_set* value, bool async,algo_req *const req){	

  PageTableEntry pte = pageTable[PPA];

  int tag = waitIdleReadBuffer();
  readTagTable[tag].req = req;

#ifdef TAGPRINT
  static int cnt = 0;
  pthread_mutex_lock(&busy_lock);
  busy_cnt++;
  if(cnt == 0)
      fprintf(stderr, "busy cnt : %d\n", busy_cnt);
  cnt = (cnt+1)%32;
  pthread_mutex_unlock(&busy_lock);
#endif

  readTagTable[tag].card = pte.card;
  readTagTable[tag].bus = pte.bus;
  readTagTable[tag].chip = pte.chip;
  readTagTable[tag].block = pte.block;
  readTagTable[tag].page = pte.page;

  enqRequest(FS_GET_T, pte.card, pte.bus, pte.chip, pte.block, pte.page, tag);
  return NULL;
}

void *vcu_trim_block(uint32_t PPA, bool async){
    //FIXME: erase cache when trim
  PageTableEntry pte = pageTable[PPA];

  int tag = waitIdleEraseTag();

  eraseTagTable[tag].card = pte.card;
  eraseTagTable[tag].bus = pte.bus;
  eraseTagTable[tag].chip = pte.chip;
  eraseTagTable[tag].block = pte.block;

  enqRequest(FS_DELETE_T, pte.card, pte.bus, pte.chip, pte.block, 0, tag);
  return NULL;
}

void vcu_stop(){}

void vcu_flying_req_wait(){};
