/* Block Manager Interface */
#include "BM.h"

/* Interface Functions for editing blockArray */
int32_t		BM_IsValidPage(Block* blockArray, PPA_T PPA) 
{
	/*
	 * Return whether parameter PPA is VALID or INVALID
	 * if valid -> return=1
	 * if invalid -> return=0
	 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint32_t offset = PPA % PagePerBlock;

	uint32_t index = offset / 8;
	offset = offset % 8;

	if (blockArray[PBA].ValidP[index] & ((uint8_t)1<<offset))
		return 1; // is valid
	else
		return 0; // is invalid
}

int32_t		BM_ValidatePage(Block* blockArray, PPA_T PPA)
{
	/*
	 * if valid -> do nothing, return=0
	 * if invalid -> Update ValidP and numValid, return=1
	 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint32_t offset = PPA % PagePerBlock;

	uint32_t index = offset / 8;
	offset = offset % 8;
	uint8_t off_num = (uint8_t)1<<offset;

	if (blockArray[PBA].ValidP[index] & (off_num)) // is valid?
		return 0;
	else { // is invalid. Do Validate.
		blockArray[PBA].ValidP[index] |= (off_num);
		//blockArray[PBA].Invalid--;
		return 1;
	}
}

int32_t		BM_InvalidatePage(Block* blockArray, PPA_T PPA)
{
	/*
	 * if valid -> Update ValidP and numValid, return=1
	 * if invalid -> do nothing, return=0
	 */
	PBA_T PBA = BM_PPA_TO_PBA(PPA);
	uint32_t offset = PPA % PagePerBlock;

	uint32_t index = offset / 8;
	offset = offset % 8;
	uint8_t off_num = (uint8_t)1<<offset;

	if (blockArray[PBA].ValidP[index] & (off_num)) { // is valid?
		blockArray[PBA].ValidP[index] &= ~(off_num);
		blockArray[PBA].Invalid++;
		return 1;
	}
	else  // is invalid.
		return 0;
}

int8_t		BM_ValidateBlock(Block* blockArray, PBA_T PBA)
{
	int numItem = BM_GetnumItem(); // number of ValidP elements

	for (int j=0; j<numItem; ++j)
		blockArray[PBA].ValidP[j] = BM_VALIDPAGE;
	blockArray[PBA].Invalid = 0;
	return (0);
}

int8_t		BM_InvalidateBlock(Block* blockArray, PBA_T PBA)
{
	int numItem = BM_GetnumItem(); // number of ValidP elements

	memset(blockArray[PBA].ValidP, BM_INVALIDPAGE, numItem);
	blockArray[PBA].Invalid = PagePerBlock;
	return (0);
}

int32_t		BM_ValidateAll(Block* blockArray)
{
	/* Validate All pages */
	int numItem = BM_GetnumItem(); // number of ValidP elements

	for (int i=0; i<numBlock; ++i) {
		for (int j=0; j<numItem; ++j)
			blockArray[i].ValidP[j] = BM_VALIDPAGE;
		blockArray[i].Invalid = 0;
	}
	return (0);
}

int32_t		BM_InvalidateAll(Block* blockArray)
{
	/* Invalidate All pages */
	int numItem = BM_GetnumItem(); // number of ValidP elements

	for (int i=0; i<numBlock; i++) {
		memset(blockArray[i].ValidP, BM_INVALIDPAGE, numItem);
		blockArray[i].Invalid = PagePerBlock;
	}
	return (0);
}