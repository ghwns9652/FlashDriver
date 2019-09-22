#include "../../include/container.h"

#define LG_PAGES_PER_BLOCK 8
#define LG_BLOCKS_PER_CHIP 12
#define LG_CHIPS_PER_BUS 3
#define LG_NUM_BUSES 3
#define LG_NUM_CARDS 1

typedef enum {        
    UNINIT,             
    ERASED,             
    ERASED_BAD,         
    WRITTEN             
} FlashStatusT;       

typedef struct {      
    bool busy;          
    int card;           
    int bus;            
    int chip;           
    int block;          
    int page;           
    algo_req* req;      
} TagTableEntry;      

typedef struct {      
    bool inflight;    
    int card;         
    int bus;          
    int chip;         
    int block;        
    int page;         
} PageTableEntry;     

struct vcu_request{   
    FSTYPE type;      
    int card;         
    int bus;          
    int chip;         
    int block;        
    int page;         
    int tag;          
};                    

uint32_t vcu_create(lower_info*);
void *vcu_destroy(lower_info*);
void* vcu_push_data(uint32_t ppa, uint32_t size, value_set *value,bool async, algo_req * const req);
void* vcu_pull_data(uint32_t ppa, uint32_t size, value_set* value,bool async,algo_req * const req);
void* vcu_trim_block(uint32_t ppa,bool async);
void* vcu_make_push(uint32_t ppa, uint32_t size, value_set *value,bool async, algo_req * const req);
void* vcu_make_pull(uint32_t ppa, uint32_t size, value_set* value,bool async,algo_req * const req);
void* vcu_make_trim(uint32_t ppa,bool async);
void *vcu_refresh(lower_info*);
void vcu_stop();
void vcu_flying_req_wait();
