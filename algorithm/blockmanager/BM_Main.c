#include "BM.h"

Block* BlockArray;
Heap* heeeeeap;

int main(void)
{
	printf("BM and Heap init\n");
    BM_Init(&BlockArray);
	heeeeeap = BM_Heap_Init(_NOS);

	printf("Heap add\n");

	for(int i = 0; i < _NOS; i++){
		BM_Heap_Insert(heeeeeap, &BlockArray[i]);
	}

	printf("Heap print\n");

	heap_print(heeeeeap);

	printf("add rand value\n");

	for(int i = 0; i < _NOS; i++){
		BlockArray[i].Invalid = rand() % _PPS;
	}

	printf("Heap print\n");

	heap_print(heeeeeap);

	printf("Heapify\n");

	max_heapify(heeeeeap);

	printf("Heap print\n");

	heap_print(heeeeeap);

	BM_Heap_Free(heeeeeap);
	BM_Free(BlockArray);
}
