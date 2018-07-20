/* Block Manager Interface */
#include "BM.h"

/* Interface Functions for editing blockArray */
int32_t	BM_IsValidPage(BM_T* BM, PPA_T PPA){
	/*
	 * Return whether parameter PPA is VALID or INVALID
	 * if valid -> return=1
	 * if invalid -> return=0
	 */
	Block* blockArray = BM->barray;
	PBA_T PBA = PPA/PagePerBlock;
	uint32_t offset = PPA % PagePerBlock;
	uint32_t index = offset / 8;
	offset = offset % 8;

	if(blockArray[PBA].ValidP[index] & ((uint8_t)1<<offset)){
		return 1; // is valid
	}
	else{
		return 0; // is invalid
	}
}

int32_t	BM_ValidatePage(BM_T* BM, PPA_T PPA){
	/*
	 * if valid -> do nothing, return=0
	 * if invalid -> Update ValidP and numValid, return=1
	 */
	Block* blockArray = BM->barray;
	PBA_T PBA = PPA/PagePerBlock;
	uint32_t offset = PPA % PagePerBlock;
	uint32_t index = offset / 8;
	offset = offset % 8;

	if(blockArray[PBA].ValidP[index] & ((uint8_t)1<<offset)){ // is valid?
		return 0;
	}
	else { // is invalid. Do Validate.
		blockArray[PBA].ValidP[index] |= ((uint8_t)1<<offset);
		return 1;
	}
}

int32_t	BM_InvalidatePage(BM_T* BM, PPA_T PPA){
	/*
	 * if valid -> Update ValidP and numValid, return=1
	 * if invalid -> do nothing, return=0
	 */
	Block* blockArray = BM->barray;
	PBA_T PBA = PPA/PagePerBlock;
	uint32_t offset = PPA % PagePerBlock;

	uint32_t index = offset / 8;
	offset = offset % 8;

	if (blockArray[PBA].ValidP[index] & ((uint8_t)1<<offset)) { // is valid?
		blockArray[PBA].ValidP[index] &= ~((uint8_t)1<<offset);
		blockArray[PBA].Invalid++;
		return 1;
	}
	else  // is invalid.
		return 0;
}

int32_t	BM_GC_InvalidatePage(BM_T* BM, PPA_T PPA){
	/*
	 * if valid -> Update ValidP and numValid, return=1
	 * if invalid -> do nothing, return=0
	 */
	Block* blockArray = BM->barray;
	PBA_T PBA = PPA/PagePerBlock;
	uint32_t offset = PPA % PagePerBlock;

	uint32_t index = offset / 8;
	offset = offset % 8;

	if (blockArray[PBA].ValidP[index] & ((uint8_t)1<<offset)) { // is valid?
		blockArray[PBA].ValidP[index] &= ~((uint8_t)1<<offset);
		return 1;
	}
	else  // is invalid.
		return 0;
}

int8_t BM_ValidateBlock(BM_T* BM, PBA_T PBA){
	Block* blockArray = BM->barray;

	for (int i = 0; i < numBITMAPB; i++){
		blockArray[PBA].ValidP[i] = BM_VALIDPAGE;
	}
	blockArray[PBA].Invalid = 0;
	return 0;
}

int8_t BM_InvalidateBlock(BM_T* BM, PBA_T PBA){
	Block* blockArray = BM->barray;

	memset(blockArray[PBA].ValidP, BM_INVALIDPAGE, numBITMAPB);
	blockArray[PBA].Invalid = PagePerBlock;
	return 0;
}

int32_t BM_ValidateAll(BM_T* BM){
	/* Validate All pages */
	Block* blockArray = BM->barray;

	for (int i = 0; i < numBlock; i++){
		for (int j = 0; j < numBITMAPB; j++){
			blockArray[i].ValidP[j] = BM_VALIDPAGE;
		}
		blockArray[i].Invalid = 0;
	}
	return 0;
}

int32_t	BM_InvalidateAll(BM_T* BM){
	/* Invalidate All pages */
	Block* blockArray = BM->barray;

	for (int i = 0; i < numBlock; i++){
		memset(blockArray[i].ValidP, BM_INVALIDPAGE, numBITMAPB);
		blockArray[i].Invalid = PagePerBlock;
	}
	return 0;
}