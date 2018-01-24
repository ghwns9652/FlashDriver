/* Badblock Manager */
#include "BM_Interface.h"


#define numBlock 10		// '현재 멀쩡한 Block의 개수' 를 반환해야 한다. 어떻게 할 지 찾아보자.

// 기본 4가지 인터페이스

void		BM_invalidate_ppa(Block* ptrBlock[], uint32_t PPA)
{
	// array를 통해 invalid 여부를 표시
	// parameter로 받은 PPA를 VALID에서 INVALID로 바꾸는 함수인듯]
	
	/*
	 * 어.. 이거.... parameter를 뭐로 해야되지??? 기본적으로 ptrBlock이 어떤 순으로 정렬되어있지? 만약 PPA 순으로 정렬되어있다면 PPA로 바로
	 * index 접근할 수 있는데.. ptrBlock[PPA] 이렇게.. 그런데 만약 그렇게 정렬되어있지 않다면
	 * PPA를 통해 일단 그게 ptrBlock의 몇 번째 index에 있는지 검색하여 찾아야 한다.. 그런데 검색하면 성능이 좀... 
	 */

}
int32_t		BM_is_invalid_ppa(Block* ptrBlock[], uint32_t size, uint32_t PPA) // ptrBlock을 어느 영역에 두어야 하지? 일단 다 parameter로 넣자
{
	// parameter로 받은 PPA가 VALID인지 INVALID인지 반환하는 함수인듯
	// 1bit로 하게 해도 되겠지만 status가 VALID INVALID 외에 더 있을수도 있으므로 일단 char로 반환

	int32_t Block_index = BM_Find_BlockPlace_by_PPA(ptrBlock, size, PPA);	/* Index of ptrBlock[] corresponded to input PPA */
	uint8_t offset = PPA % PPB;						/* Page offset of input PPA(Range: 0~255) */
	Block* targetBlock = ptrBlock[Block_index];		/* Pointer of Target Block */
	uint8_t index;									/* bit index (0,1,2,3) */
	uint64_t targetBit;								/* Validity of parameter PPA(1 means VALID, 0 means INVALID) */


	/* Find the offset(index) of target bit */
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
	offset = offset % 64;	/* Example: page offset 253 becomes 253-192 = 61 */


	/* 
	 * Now, Range of offset is 0~63 
	 * targetBit indicates 'Validity of parameter PPA'. 1 means VALID, 0 means INVALID
	 */
	targetBit = targetBlock->bit[index] << 64 - (offset + 1); 
	targetBit = targetBit >> 63;								// Didn't merge two lines for more readability
	//이거는 그냥 validity 확인할 뿐인데.. 그 bit만 딱 바꾸려면 OR 연산 해야할듯?

	if (!targetBit)
		return 1; // INVALID
	else
		return 0; // VALID


}
uint32_t	BM_get_gc_victim(Block *ptrBlock[], uint32_t size)
{
	/* victim block의 PBA를 반환하는 함수 */
	/* 
	 * Parameter: Array(Heap) of Block structure
	 * Parameter가 Heap으로 주어지므로, Heap 연산을 이용하여 cnt(P/E)가 max인 node를(max heap의 root) 찾아서 그 PPA(PBA?)를 반환한다.
	 * 아니 PPA가 아니라 Block 자체를 반환하는 건가?
	 */

	/* Make Max-heap by invalid count, cnt */
	//build_max_heap_cnt(Block_list, numBlock); // numBlock 말고 진짜 size 넣어야..
	build_max_heap_cnt_(ptrBlock, size);

	// 음... cnt 기준으로 바꾼다 한들.. 단순히 SWAP하는 게 아니라 'cnt가 가장 높은 block에 해당하는 PBA'를 찾아야 한다. cnt array와 PBA array는 index가 다르다!!
	// 지금은 안되어있는데 고쳐야 한다..

	/* 0th offset of cnt array is the block of maximum cnt.. but is it corresponding PBA? */
	return ptrBlock[0]->PBA;

}
uint32_t	BM_get_weared_block(Block *ptrBlock[], uint32_t size)
{
	// 이거 뭐지
	return 0;
}