/* Badblock Manager */
#include "BM_Interface.h"

#define METHOD 1	// Which method has better performance?

// 기본 4가지 인터페이스

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
int32_t		BM_is_invalid_ppa(Block* blockArray, uint32_t PPA) // ptrBlock을 어느 영역에 두어야 하지? 일단 다 parameter로 넣자
{
	// parameter로 받은 PPA가 VALID인지 INVALID인지 반환하는 함수인듯
	// 1bit로 하게 해도 되겠지만 status가 VALID INVALID 외에 더 있을수도 있으므로 일단 char로 반환

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
	/* victim block의 PBA를 반환하는 함수 */
	/*
	* Parameter: Array(Heap) of numValid pointer(numValid_map)
	* Parameter가 Heap으로 주어지므로, Heap 연산을 이용하여 numValid가 max인 node를(max heap의 root) 찾아서 그 PBA를 반환한다.
	*/


	/* After this function, numValid_map will become Max-heap by numValid */
	BM_Maxheap_numValid(blockArray, numValid_map);

	/* Make Block_pointer from numValid_pointer */
	void* ptr_max_nV_block = numValid_map[0] - sizeof(ValidP_T)*NOP - sizeof(PBA_T);

	return *((PBA_T*)ptr_max_nV_block); // This means value of PBA of maxnV block

}
uint32_t	BM_get_worn_block(Block *blockArray, uint32_t* PE_map[])
{
	/* PE_map을 PE_cycle 순서대로 Ascending order sorting하는 함수 */
	/*@
	 * Parameter: Array of PE_cycle pointer(PE_map)
	 * PE_map을 정렬하면 
	 * 실제 Flash 내의 data를 SWAP하는 과정도 들어가야 할 것
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
	
	BM_SortPE(blockArray, PE_map);
	return 0;
}

int32_t BM_update_block_with_gc(Block* blockArray, uint32_t PPA)
{
	/* GC할 때마다 불러야 하는 함수 */
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

inline void BM_update_block_with_push(Block* blockArray, uint32_t PPA)
{
	/* Push할 때마다 불러야 하는 함수 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint8_t offset = PPA % PPB;

	blockArray[PBA].ValidP[offset] = BM_WRITTEN; /* Not Determined yet */

	blockArray[PBA].PE_cycle++;
}
inline void BM_update_block_with_trim(Block* blockArray, uint32_t PPA)
{
	/* Trim할 때마다 불러야 하는 함수 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);


	blockArray[PBA].PE_cycle++;


}

추가할 것
GC할 때마다 부르는 함수
GC하면.. numValid==0이고 모든 ValidP[]도 VALID 상태가 되겠지(ERASE를 따로 만들어야 하나?)

free block pool 쪽에서도 뭔가 해야할 것 같은데.. 해당 block을 free block pool에 넣어야 하나?


PE cycle 늘리는 거, numValid 늘리는 거, ValidP[]를 바꾸는 것 등... 인터페이스 함수 필요할까? 한줄이면 되겠지만 inline으로다가 주면 좋을듯?