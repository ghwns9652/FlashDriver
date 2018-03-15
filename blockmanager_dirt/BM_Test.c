
#if 0
#include "BM_Interface.h"



#include "BM_Heap.h" // 임시

#ifdef DEBUG
int32_t setblock(Block targetBlock, uint8_t offset) 	// return type이 ERR type과 맞나?
{
	printf("PBA: %d\n", targetBlock.PBA);
	printf("offset: %d\n", offset);

	uint8_t index;

	/* Range of offset: 0~255 */
	if (offset < 128) {
		if (offset > 64) { index = 1; }
		else if (offset > 0) { index = 0; }
		else { ERR(eBADOFFSET_BM); }
	}
	else {
		if (offset < 192) { index = 2; }
		else if (offset < 256) { index = 3; }
		else { ERR(eBADOFFSET_BM); }
	}

	offset = offset % 64;	/* offset 253 becomes 253-192=61 */

							/* Now, Range of offset is 0~63 */
	uint64_t bit = targetBlock.bit[index] << 64 - (offset + 1);
	bit = bit >> 63;
	//이거는 그냥 validity 확인할 뿐인데.. 그 bit만 딱 바꾸려면 OR 연산 해야할듯?

	// 그 bit가 1이면 Valid한 것.
	if (!bit)
		return 1; // INVALID
	else
		return 0; // VALID

				  /*
				  if (offset >= 0 && offset < 64)
				  index = 1;
				  else if (offset < 128)
				  index = 2;
				  else if (offset < 192)
				  index = 3;
				  else if (offset < 256)
				  index = 4;
				  else
				  ERR(eBADOFFSET_BM);	// return type이 ERR type과 맞나?
				  return(eNOERROR);
				  */
}
#endif // DEBUG


// #if 0 ~ #endif 는 주석처리하는 것
#if 0 
void print_binary(uint64_t num)
{
	int i;
	uint64_t bit = 0x80000000;

	for (i = 0; i < 32; ++i) {
		if (i != 0 && i % 4 == 0)	printf(" ");
		if (num & bit)	printf("1");
		else printf("0");
		num = num << 1;
	}
	printf("\n");
}
#endif

int32_t main(void)
{
	// 배열이 여러 개 독립적으로 들어있는 포인터가 Block

	Block* arr[10]; // 구조체 포인터 배열

	printf("size: %d\n", sizeof(arr)/sizeof(Block*));


	for (int i = 0; i < 10; ++i) {
		arr[i] = malloc(sizeof(Block));

		arr[i]->PBA = i;
		arr[i]->numValid = i;
		arr[i]->PE_cycle = rand() % 10;
		for (int j=0; j<4; ++j)
			arr[i]->bit[j] = i;
		
	}
	print_arr_all(arr, sizeof(arr)/sizeof(Block*));
	int size = sizeof(arr) / sizeof(Block*);
	
	puts("---------------------");
	build_max_heap_cnt_(arr, size);

	//print_arr_cnt(arr, size);

	print_arr_all(arr, size);
	//Block* ptrtemp = arr[0];
	//arr[0] = arr[1];
	//arr[1] = ptrtemp;

	SWAP_CNT(arr[0], arr[1]);

	print_arr_all(arr, BSIZE(arr));

	//setblock(*(arr[3]), 1);

	printf("number: %d\n", 0x401);
	//for (unsigned int i = size / 2; i >= 0; --i) {// unsigned 하면 안된다
	//	printf("i: %d\n", i);
	//}
	/*
	int32_t arr[10] = { 4,1,2,5,8,6,3,12,62,43 };

	print_arr(arr, 10);
	//max_heapify(arr, 10, 0);
	//heapSort(arr, 10);
	build_max_heap(arr, 10);
	print_arr(arr, 10);

	//realloc 실험
	int* numarray = (int*)malloc(sizeof(int) * 10);
	for (int i = 0; i < 10; ++i)
		numarray[i] = i;
	print_arr(numarray, 10);
	numarray = (int*)realloc(numarray, sizeof(int) * 9);
	print_arr(numarray, 10);

	//ERR(eHEAPUNDERFLOW_BM);

	build_max_heap(numarray, 9);
	print_arr(numarray, 9);
	printf("Maximum value: %d\n", heap_maximum_extract(numarray, 9));
	print_arr(numarray, 9);
	*/
}

#endif