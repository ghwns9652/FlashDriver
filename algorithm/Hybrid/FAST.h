#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../../include/container.h"

/*@
 * Structure Definition
 */
typedef struct {
    request* parents;
} FAST_Parameters;

typedef struct {
    uint32_t logical_block;
    uint32_t physical_block;
    uint32_t number_of_stored_sector;
} SW_MappingInfo;

typedef struct {
    SW_MappingInfo* data;       /* Should allocate with value of lower_info */
} SW_MappingTable;

typedef struct {
    uint32_t physical_block;
    uint32_t logical_block;
    uint32_t physical_offset;
    uint32_t logical_offset;
    char state;
} RW_MappingInfo;

typedef struct {
    uint32_t* rw_log_block;
    char current_position;
    uint32_t number_of_full_log_block;
    uint32_t offset;
    RW_MappingInfo* data;       /* Should allocate with value of lower_info */
} RW_MappingTable;

typedef struct {
    uint32_t physical_block;
} Block_MappingInfo;

typedef struct {
    Block_MappingInfo* data;    /* Should allocate with the value of lower_info */
} Block_MappingTable;

typedef struct {
    SW_MappingTable* sw_MappingTable;
    RW_MappingTable* rw_MappingTable;
    Block_MappingTable* block_MappingTable;
} TableInfo;

/*@
 * Function Prototypes
 */

/* API Function */
uint32_t FAST_Create(lower_info* li, algorithm* algo);
void FAST_Destroy(lower_info* li, algorithm* algo);
uint32_t FAST_Get(request* const req);
uint32_t FAST_Set(request* const req);
uint32_t FAST_Remove(request* const req);
void* FAST_EndRequest(algo_req* input);

/* Internal Function */
/* FAST_Create */
int fast_AllocRWLogBlock();
int fast_AllocSWLogBlock();

/* FAST_Get */
int fast_ReadFromDataBlock(uint32_t physical_address);
int fast_ReadFromSWLogBlock(uint32_t physical_address);
int fast_ReadFromRWLogBlock(uint32_t physical_address);

/* Fast_Set */
char fast_Write();
char fast_WriteToLogBlock();
char fast_AllocDataBlockEntry(KEYT key, uint32_t* physical_address);
char fast_AllocSWLogBlockEntry(KEYT key, uint32_t* physical_address);
char fast_AllocRWLogBlockEntry(KEYT key, uint32_t* physical_address);

/* FAST_Remove */
char fast_SwitchSWLogBlock(uint32_t log_block_number);
char fast_MergeSWLogBlock(uint32_t log_block_number);
char fast_MergeRWLogBLock(uint32_t log_block_number);

char fast_SearchSWLogBlock(uint32_t logical_address, uint32_t* physical_address);
char fast_SearchRWLogBlock(uint32_t logical_address, uint32_t* physical_address);
char fast_SearchDataBlock(uint32_t logical_address, uint32_t* physical_address);

/* Commonly Used Function */
uint32_t BLOCK(uint32_t logical_address);
uint32_t OFFSET(uint32_t logical_address);
uint32_t ADDRESS(uint32_t block, uint32_t offset);
uint32_t BLOCK_TABLE(uint32_t logical_block);

char GET_PAGE_STATE(uint32_t physical_address);
void SET_PAGE_STATE(uint32_t physical_address, char state);
char GET_BLOCK_STATE(uint32_t physical_address);
void SET_BLOCK_STATE(uint32_t physical_address, char state);

/*@
 * Global Variables
 */

extern const char NUMBER_OF_RW_LOG_BLOCK;

extern const char ERASED;
extern const char VALID;
extern const char INVALID;

extern const char eNOERROR;
extern const char eNOTWRITTEN;
extern const char eOVERWRITTED;
extern const char eUNEXPECTED;
extern const char eNOTSEQUENTIAL;

extern const char eNOTFOUND;
extern const char UNEXPECTED;

extern const char NIL;

extern struct algorithm FAST_Algorithm;
extern TableInfo* tableInfo;
extern char* BLOCK_STATE;
extern char* PAGE_STATE;

extern uint32_t NUMBER_OF_BLOCK;
extern uint32_t NUMBER_OF_PAGE;
extern uint32_t SIZE_OF_KEY_TYPE;
extern uint32_t SIZE_OF_BLOCK;
extern uint32_t PAGE_PER_BLOCK;
extern uint32_t TOTAL_SIZE;

extern uint32_t NUMBER_OF_DATA_BLOCK;

extern const char DATA_BLOCK;
extern const char SW_LOG_BLOCK;
extern const char RW_LOG_BLOCK;

extern algorithm FAST_Algorithm;
