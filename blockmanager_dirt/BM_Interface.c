/* Badblock Manager */
#include "BM_Interface.h"

#define METHOD 1	// Which method has better performance?

// Four basic interfaces

int32_t		BM_invalidate_ppa(Block* blockArray, uint32_t PPA)
{
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % PPB;

#if (METHOD == 1)
	blockArray[PBA].ValidP[offset] = BM_INVALIDPAGE;
	return (eNOERROR);
#endif

#if (METHOD == 2)
	if (blockArray[PBA].ValidP[offset] == BM_INVALIDPAGE) {
		printf("Input PPA is already INVALID\n");
		return (eNOERROR);
	}
	else {
		blockArray[PBA].ValidP[offset] = BM_INVALIDPAGE;
		return (eNOERROR);
	}
#endif
}
int32_t		BM_is_invalid_ppa(Block* blockArray, uint32_t PPA) 
{
	/*
	 * Return whether parameter PPA is VALID or INVALID
	 * Current Implementation: using char array to express state
	 */

	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % PPB;


	/* if - else if should be switched if invalid page is more than valid page */
	if (blockArray[PBA].ValidP[offset] == BM_VALIDPAGE) {
		printf("Input PPA is VALID\n");
		return (0);
	}
	else if (blockArray[PBA].ValidP[offset] == BM_INVALIDPAGE) {
		printf("Input PPA is UNVALID\n");
		return (1);
	}
	else {
		printf("Error!\n");
		ERR(eBADVALIDPAGE_BM);
	}
}
uint32_t	BM_get_gc_victim(Block* blockArray, uint8_t* numValid_map[])
{
	/* Return PBA of victim block */
	/*
	 * Parameter: Array(Heap) of numValid pointer(numValid_map)
	 * numValid_map is a heap array, so find the root of the max-heap in numValid using Heap opeartion
	 */


	/* After this function, numValid_map will become Max-heap by numValid */
	BM_Maxheap_numValid(blockArray, numValid_map);

	/* Make Block_pointer from numValid_pointer */
	void* ptr_max_nV_block = numValid_map[0] - sizeof(ValidP_T)*NOP - sizeof(PBA_T);

	return *((PBA_T*)ptr_max_nV_block); // This means value of PBA of maxnV block

}

uint32_t	BM_get_minPE_block(Block* blockArray, uint8_t* PE_map[])
{
	/* Return PBA of minPE block whose PE_cycle is minimum */
	/*
	 * Parameter: Array(Heap) of PE_map pointer(PE_map)
	 * PE_map is a Min-heap array, so find the root of the min-heap in PE_cycle using Heap opeartion
	 */


	/* After this function, PE_map Min-heap by PE_cycle */
	BM_Minheap_PEcycle(blockArray, PE_map);

	/* Make Block_pointer from PE_cycle pointer */
	void* ptr_min_PE_block = PE_map[0] - sizeof(nV_T) - sizeof(ValidP_T)*NOP - sizeof(PBA_T);

	return *((PBA_T*)ptr_min_PE_block); // This means value of PBA of minPE block

}


uint32_t	BM_get_worn_block(Block *blockArray, uint32_t* PE_map[])
{
	/* Function which sorts PE_map by PE_cycle with ascending order
	/*@
	 * Parameter: Array of PE_cycle pointer(PE_map)
	 * (Warning) We need 'SWAP REAL DATA IN FLASH'. Current codes have no this step.
	 */

	/* 어떤 식으로 할까? 
	1. PE_cycle 순서대로 sorting해서 그 순서대로 아예 PBA를 바꿔버리기 
	2. 가쟝 PE_cycle이 높은 block과 가장 PE_cycle이 낮은 block의 데이터를 교환하기(Heap으로 하는 게 낫을려나)(물론 둘 다 Badblock이 아닌 걸로)
	*/
	
	
	/* 교수님 생각 */
	/*
	 * wear-leveling은 가끔 일어나는 작업이고, 우리 연구주제는 wear-leveling 자체가 아니니까 간단하게 구현하도록 한다.
	 * free block pool을 만들 때, 그 allocation 대상을 PE_cycle이 가장 낮은 block을 return해서 하는 방식으로
	 * max와 min의 차이가 어느 정도 일어날 때 wear-leveling이 일어나거나 등..
	 */
	// 그런데 free block pool은 어디서 관리되지? 경택이인가?
	
	/* Sort PE_map by PE_cycle */
	BM_SortPE(blockArray, PE_map);
	


	return (eNOERROR);
}

int32_t BM_update_block_with_gc(Block* blockArray, uint32_t PPA)
{
	/* This function should be called when GC
	/*
	 * Parameter: PPA(or PBA?)
	 * Update status of corresponding block
	 * numValid = 0, ValidP[] = 0
	 */

	PBA_T PBA = BM_PPA_TO_PBA(PPA);

	blockArray[PBA].numValid = 0;
	memset(blockArray[PBA].ValidP, BM_VALIDPAGE, NOP);

	return (eNOERROR);
}

inline int32_t BM_update_block_with_push(Block* blockArray, uint32_t PPA)
{
	/* This function should be called when Push */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % PPB;

	blockArray[PBA].ValidP[offset] = BM_WRITTEN; /* Not Determined yet */ /* What is BM_WRITTEN? */

	blockArray[PBA].PE_cycle++;
}
inline int32_t BM_update_block_with_trim(Block* blockArray, uint32_t PPA)
{
	/* This function should be called when Trim */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);

	blockArray[PBA].PE_cycle++;


}

/*
추가할 것
GC할 때마다 부르는 함수
GC하면.. numValid==0이고 모든 ValidP[]도 VALID 상태가 되겠지(ERASE를 따로 만들어야 하나?)

free block pool 쪽에서도 뭔가 해야할 것 같은데.. 해당 block을 free block pool에 넣어야 하나?


PE cycle 늘리는 거, numValid 늘리는 거, ValidP[]를 바꾸는 것 등... 인터페이스 함수 필요할까? 한줄이면 되겠지만 inline으로다가 주면 좋을듯?
*/
