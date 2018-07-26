#include "block.h"

void GC_moving(request *const req, algo_req* my_req, uint32_t LBA, uint32_t offset, uint32_t PBA, uint32_t PPA, int8_t checker)
{
	/* Allocate an empty block */
#ifdef Queue
	set_pointer = Dequeue(FreeQ);
#endif
#ifdef Linear
	checker = block_findsp(checker);
#endif

	/* Maptable updates for data moving */
	block_maptable[LBA] = set_pointer;
	block_valid_array[set_pointer] = EXIST;
	block_valid_array[PBA] = ERASE; // PBA means old_PBA

	uint32_t old_PPA_zero = PBA * ppb_;
	uint32_t new_PBA = block_maptable[LBA];
	uint32_t new_PPA_zero = new_PBA * ppb_;
	uint32_t i = 0;
	//printf("moving start! LBA: %d, offset: %d, old_PBA: %d, new_PBA: %d ------------------------------------------\n", LBA, offset, PBA, new_PBA);

	/* Start move */
#ifdef BFTL_DEBUG1
	printf("Start move!\n");
#endif

	numLoaded = 0;
	value_set** temp_set = (value_set**)malloc(sizeof(value_set*)*ppb_);
	sram_valueset = (value_set*)malloc(sizeof(value_set) * ppb_);
	memset(sram_valueset, 0, sizeof(value_set) * ppb_);
	//sleep(2);

	//if (offset % 4 == 0)
		//printf("in GC, offset: %d\n", offset);
	for (i=0; i<ppb_; ++i) { // non-empty page만 옮겨야 하지 않을까? 이것도 상관은 없지만..
#ifdef BFTL_DEBUG3
		if (i % EPOCH == 0)
			printf("SRAM_load: %d\n", i);
#endif
		// SRAM_load
		if (i != offset) {
			temp_set[i] = SRAM_load(i, old_PPA_zero);
#ifdef BFTL_DEBUG2
			printf("temp_set[%d]->value: %c\n", i, *(temp_set[i]->value));
#endif
		}
	}
	//printf("numLoaded: %d\n", numLoaded);

	while (numLoaded != ppb_ - 1) {} // polling for reading all pages in a block

	for (i=0; i<ppb_; ++i) {
#ifdef BFTL_DEBUG3
		if (i % EPOCH == 0)
			printf("memcpy: %d\n", i);
#endif
		// copy data 
		if (i != offset) {
			memcpy(&sram_valueset[i], temp_set[i], sizeof(value_set));
			/*
			   else {
			   memcpy(&sram_valueset[i], req->value, sizeof(value_set)); // req->value: value_set*
			   printf("target page offset: %d\n", i);
			   }
			 */
#ifdef BFTL_DEBUG2
			printf("sram_valueset[%d].value: %c\n", i, *(sram_valueset[i].value));
#endif
			//printf("sram_valueset[%d].length: %u\n", i, (sram_valueset[i].length));
		}
	}


	for (i=0; i<ppb_; ++i) {
#ifdef BFTL_DEBUG3
		if (i % EPOCH == 0)
			printf("SRAM_unload: %d\n", i);
#endif
		// SRAM_unload
		BM_ValidatePage(BM, new_PPA_zero + i);
		BM_InvalidatePage(BM, old_PPA_zero + i);
		if (i != offset)
			SRAM_unload(i, new_PPA_zero);
		else
			__block.li->push_data(new_PPA_zero + offset, PAGESIZE, req->value, ASYNC, my_req);

	}
	for (i=0; i<ppb_; ++i)
		if (i != offset)
			inf_free_valueset(temp_set[i], FS_MALLOC_R);

	//printf("numLoaded: %d\n", numLoaded);
	/* Trim the block of old PPA */
#ifdef BFTL_DEBUG1
	printf("trim!\n");
#endif
	__block.li->trim_block(old_PPA_zero, false);
#ifdef Queue
	Enqueue(FreeQ, PBA);
#endif
#ifdef BFTL_DEBUG1
	printf("trim end!\n");
#endif
	free(sram_valueset);
}
