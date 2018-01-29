#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NOP (10)

//#define PBA_T uint32_t
typedef uint32_t	PBA_T;


typedef struct {
	uint32_t PBA;
	int8_t bit[NOP];
	uint8_t numValid;
	uint32_t PE_cycle;
	uint8_t* ptrNV_data;
	uint32_t* ptrPE_data;
} Block;

int main(void)
{
	Block A[NOP];
	printf("start\n");
	A[3].PBA = 42;
	memset(A[3].bit, 0, NOP);
	A[3].numValid = 86;
	A[3].PE_cycle = 14;

	uint8_t* ptr_nV = &(A[3].numValid);
	uint32_t* ptr_PBA = &(A[3].PBA);
	int8_t* ptr_bit = &(A[3].bit);
	Block* ptr_A3 = &A[3];
	
	printf("ptr_A: %p\n", (void*)ptr_A3);
	printf("Address of A: %p\n", (void*)(&A));
	printf("Value of ptr_nV: %d\n", *ptr_nV);
	printf("ptr_PBA: %p\n", (void*)ptr_PBA);
	printf("ptr_bit: %p\n", (void*)ptr_bit);
	printf("ptr_nV: %p\n", (void*)ptr_nV);
	
	printf("ptr_A   - ptr_PBA = %p\n", (void*)ptr_A3-(void*)ptr_PBA);
	printf("ptr_bit - ptr_PBA = %p\n", (void*)ptr_bit-(void*)ptr_PBA);
	printf("ptr_nV  - ptr_bit = %p\n", (void*)ptr_nV-(void*)ptr_bit);
	puts("");
	
	void* ptr_origin = ptr_nV - sizeof(A[3].bit) - sizeof(A[3].PBA);
	printf("ptr_origin: %p\n", ptr_origin);
	printf("ptr_origin to Value of PBA: %d\n", *((uint32_t*)ptr_origin));
	Block* new_ptrA = (Block*)ptr_origin;
	printf("PE_cycle: %d\n", new_ptrA->PE_cycle);

	printf("ptr_origin+3: %p\n", ptr_origin+3);
	
	uint32_t* ptr_newPBA = ptr_nV - sizeof(A[3].bit) - sizeof(A[3].PBA);
	printf("Value of ptr_newPBA: %d\n", *ptr_newPBA);

	PBA_T* macroisgood = &(A[3].PBA);
	printf("macroisgood: %d\n", *macroisgood);

	
	printf("size of one block: %d\n", sizeof(A[3]));
	printf("size of PBA: %d\n", sizeof(A[3].PBA));
	printf("size of bit: %d\n", sizeof(A[3].bit));
	printf("size of numValid: %d\n", sizeof(A[3].numValid));


}


