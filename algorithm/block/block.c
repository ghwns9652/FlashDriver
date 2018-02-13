#include <string.h>
#include <stdlib.h>
#include "block.h"

#include <stdio.h> // Temporary measure for printf

int32_t *block_maptable; // pointer to LPA->PPA table 
int8_t *exist_table; 
int8_t *block_valid_array;
uint32_t set_pointer = 0;

#define VALID 1
#define ERASE 0
#define NIL -1
#define EXIST 1
#define NONEXIST 0

struct algorithm __block={
	.create=block_create,
	.destroy=block_destroy,
	.get=block_get,
	.set=block_set,
	.remove=block_remove
};

//Set set_pointer to first-meet ERASE index from current set_pointer
int32_t block_findsp(int32_t checker){
	for (; set_pointer < __block.li->NOB; ++set_pointer) {
		if (block_valid_array[set_pointer] == ERASE) {
			checker = 1;
			break;
		}
	}
	if (checker == 0) {
		for (set_pointer =0; set_pointer < __block.li->NOB; ++set_pointer)
			if (block_valid_array[set_pointer] == ERASE){
				checker = 1;
				break;
			}
	}
	return checker;
}

uint32_t block_create (lower_info* li,algorithm *algo){
	algo->li=li;

	block_maptable = (int32_t*)malloc(sizeof(int32_t) * li->NOB);
	int32_t i=0;
	for (; i<li->NOB; ++i){ // maptable initialization
		block_maptable[i] = NIL;
	}

	exist_table = (int8_t*)malloc(sizeof(int8_t)*li->NOP);
	for (i = 0; i < li->NOP; ++i)
		exist_table[i] = NONEXIST;

	block_valid_array = (int8_t*)malloc(sizeof(int8_t)*li->NOB);
	for (i = 0; i < li->NOB; ++i)
		block_valid_array[i] = ERASE; // 0 means ERASED, 1 means VALID
	// memset(block_valid_array, 0, li->NOB * li->SOB); 

}
void block_destroy (lower_info* li, algorithm *algo){

	free(block_maptable);
	free(exist_table);
	free(block_valid_array);
}
uint32_t block_get(const request *req){
	bench_algo_start(req);
	//block_params* params=(block_params*)malloc(sizeof(block_params));
	//params->parents=req;
	//params->test=-1;

	algo_req *my_req=(algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req=block_end_req;
	//my_req->params=(void*)params;

	uint32_t LBA = my_req->parents->key / __block.li->PPB;
	uint32_t offset = my_req->parents->key % __block.li->PPB;

	uint32_t PBA = block_maptable[LBA];
	uint32_t PPA = PBA * __block.li->PPB + offset;



	//__block.li->pull_data(req->key,PAGESIZE,req->value,0,my_req,0);
	__block.li->pull_data(PPA, PAGESIZE, req->value, 0, my_req, 0);
	bench_algo_end(req);
}
uint32_t block_set(const request *req){
	bench_algo_start(req);
	//block_params* params=(block_params*)malloc(sizeof(block_params));
	//params->parents=req;
	//params->test=-1;

	algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
	my_req->parents = req;
	my_req->end_req = block_end_req;
	//my_req->params = (void*)params;

	uint32_t LBA = my_req->parents->key / __block.li->PPB;
	uint32_t offset = my_req->parents->key % __block.li->PPB;
	uint32_t PBA;
	uint32_t PPA;
	//uint32_t PPB = __block.li->PPB;
	int8_t checker = 0;


	//if (checker == 0) {
		/* There is NO free space in flash block */
		/* We need OverProvisioning area, maybe. */



	if (block_maptable[LBA] == NIL)
	{
		checker = block_findsp(checker);
		//printf("Case 1\n");
		// Switch E to V of block_valid_array
		block_valid_array[set_pointer] = VALID;

		// Write PBA of mapping result in maptable
		block_maptable[LBA] = set_pointer;

		//PBA = block_maptable[LBA];
		//PPA = PBA * PPB + offset;
		PPA = set_pointer * __block.li->PPB + offset; // Equal to above 2 lines

		exist_table[PPA] = EXIST;

		// write
		//algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
		//my_req->end_req = block_end_req;
		//my_req->params = (void*)params;
		__block.li->push_data(PPA, PAGESIZE, req->value, 0, my_req, 0);
	}

	else
	{
		PBA = block_maptable[LBA];
		PPA = PBA  * __block.li->PPB + offset;
#if 0
		if (exist_table[PPA] == NONEXIST && offset == 0)
		{
			//printf("Case 2\n");
			exist_table[PPA] = EXIST;
			block_valid_array[PBA] = VALID;
			//algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
			//my_req->end_req = block_end_req;
			//my_req->params = (void*)params;
			__block.li->push_data(PPA, PAGESIZE, req->value, 0, my_req, 0);
		}
#endif
		if (exist_table[PPA] == NONEXIST)
		{
			//printf("Case 3\n");
			exist_table[PPA] = EXIST;
			if (block_valid_array[PBA] == ERASE){
				while (1)
					printf("AAA");
			}
			block_valid_array[PBA] = VALID;
			//algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
			//my_req->end_req = block_end_req;
			//my_req->params = (void*)params;
			__block.li->push_data(PPA, PAGESIZE, req->value, 0, my_req, 0);
		}
		else if (exist_table[PPA] == EXIST) //!= NONEXIST)
		{
			//printf("Case GC\n");
			// Cleaning
			// Maptable update for data moving
			checker = block_findsp(checker);
			block_maptable[LBA] = set_pointer;
			block_valid_array[set_pointer] = VALID;
			block_valid_array[PBA] = ERASE; // PBA means old_PBA

			uint32_t old_PPA_zero = PBA * __block.li->PPB;
			uint32_t new_PBA = block_maptable[LBA];
			uint32_t new_PPA_zero = new_PBA * __block.li->PPB;
			uint32_t new_PPA = new_PBA * __block.li->PPB + offset;

			// Data move to new block
			//int8_t* temp_block = (int8_t*)malloc(sizeof(int8_t)*__block.li->PPB); // 이거 vale를 제대로 담을 수 있는 걸로 만들어야..
			//int8_t* temp_block = (int8_t*)malloc(PAGESIZE);
			int i;

			/* Followings: ASC consideartion */

			//printf("Start move\n");
			//int8_t* temp_block = (int8_t*)malloc(PAGESIZE);
			for (i = 0; i < offset; ++i) {
				algo_req *temp_req=(algo_req*)malloc(sizeof(algo_req));
				int8_t* temp_block = (int8_t*)malloc(PAGESIZE);
				//temp_req->end_req=block_end_req;
				//temp_req->params=(void*)params;
				temp_req->parents = NULL;
				temp_req->end_req = block_algo_end_req;
				//printf("Before %d-th pull_data\n", i);
				__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_block, 0, temp_req, 0);
				//printf("After pull_data\n");

				exist_table[old_PPA_zero + i] = NONEXIST;
				exist_table[new_PPA_zero + i] = EXIST;

				algo_req *temp_req2=(algo_req*)malloc(sizeof(algo_req));
				temp_req2->parents = NULL;
				temp_req2->end_req = block_algo_end_req;
				//algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
				//my_req->end_req = block_end_req;
				//my_req->params = (void*)params;
				//printf("Before %d-th push_data\n", i);
				__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_block, 0, temp_req2, 0);
				//printf("After push_data\n");
				free(temp_block);
			}

			//algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
			//my_req->end_req = block_end_req;
			//my_req->params = (void*)params;
			__block.li->push_data(new_PPA, PAGESIZE, req->value, 0, my_req, 0);
			exist_table[old_PPA_zero + offset] = NONEXIST;
			exist_table[new_PPA_zero + offset] = EXIST;
			
			if (offset < __block.li->PPB - 1) {
				for (i = offset + 1; i < __block.li->PPB; ++i) {
					algo_req *temp_req = (algo_req*)malloc(sizeof(algo_req));
					int8_t* temp_block = (int8_t*)malloc(PAGESIZE);
					//temp_req->end_req = block_end_req;
					//temp_req->params = (void*)params;
					temp_req->parents = NULL;
					temp_req->end_req = block_algo_end_req;
					__block.li->pull_data(old_PPA_zero + i, PAGESIZE, temp_block, 0, temp_req, 0);

					exist_table[old_PPA_zero + i] = NONEXIST;
					exist_table[new_PPA_zero + i] = EXIST;

					algo_req *temp_req2=(algo_req*)malloc(sizeof(algo_req));
					temp_req2->parents = NULL;
					temp_req2->end_req = block_algo_end_req;
					//algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
					//my_req->end_req = block_end_req;
					//my_req->params = (void*)params;
					__block.li->push_data(new_PPA_zero + i, PAGESIZE, temp_block, 0, temp_req2, 0);
					free(temp_block);
				}
			}

					

			/* Followings: No ASC consideration */
			/*
			for (i = 0; i<__block.li->PPB; ++i)
			{
				if (i == offset) {
					exist_table[PPA] = NONEXIST;
					exist_table[new_PPA] = EXIST;

					algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));
					my_req->end_req = block_end_req;
					my_req->params = (void*)params;
					__block.li->push_data(new_PPA, PAGESIZE, req->value, 0, my_req, 0);
				}
				else if (exist_table[i] == EXIST) {
					//temp_block[i] = read(PBA + i);
					algo_req *temp_req=(algo_req*)malloc(sizeof(algo_req));
					temp_req->end_req=block_end_req;
					temp_req->params=(void*)params;
					__block.li->pull_data(PBA*__block.li->PPB+i, PAGESIZE, temp_block+i, 0, temp_req, 0);

					exist_table[PBA * __block.li->PPB + i] = NONEXIST;
					exist_table[new_PBA * __block.li->PPB + i] = EXIST;
					algo_req *my_req = (algo_req*)malloc(sizeof(algo_req));

					my_req->end_req = block_end_req;	my_req->params = (void*)params;
					__block.li->push_data(new_PBA *__block.li->PPB + i, PAGESIZE, temp_block+i, 0, my_req, 0);
				}
			}
			*/
			//trim(PBA);
			//printf("Before trim\n");
			__block.li->trim_block(old_PPA_zero, false); // Is that right?
			//printf("After trim\n");
			//free(temp_block);
		}
	}
	bench_algo_end(req);

}
bool block_remove(const request *req){
	//	block->li->trim_block()
}

void *block_end_req(algo_req* input){
	//block_params* params=(block_params*)input->params;

	//request *res=params->parents;
	request* res = input->parents;
	res->end_req(res);

	//free(params);
	free(input);
}



/* From gyeongtaek idea */
void *block_algo_end_req(algo_req* input){
	free(input);
}
