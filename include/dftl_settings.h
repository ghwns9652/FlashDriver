#ifndef __H_SETDFTL__
#define __H_SETDFTL__

// write buffering flag
#define W_BUFF 1

// write buffering polling flag depend to W_BUFF
#define W_BUFF_POLL 1

// gc polling flag
#define GC_POLL 1

// eviction polling flag
#define EVICT_POLL 1

// max size of write buffer
#define MAX_SL 1024

// page buffering flag
#define P_BUFF (1)

// max size of 4k write buffer
#define BUF_REQ (2)
#define PB_SIZE (BUF_REQ / 2)

#endif
