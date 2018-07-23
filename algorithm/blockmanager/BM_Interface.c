/* Block Manager Interface */
#include "BM.h"

/* Macros for finding PBA from PPA */
inline PBA_T BM_PPA_TO_PBA(PPA_T PPA) {
	return PPA/PagePerBlock;
}

/* Interface Functions for editing blockArray */
int32_t		BM_IsValidPage(BM_T* BM, PPA_T PPA) 
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

	if (BM->barray[PBA].ValidP[index] & ((uint8_t)1<<offset))
		return 1; // is valid
	else
		return 0; // is invalid
}

int32_t		BM_ValidatePage(BM_T* BM, PPA_T PPA)
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

	if (BM->barray[PBA].ValidP[index] & (off_num)) // is valid?
		return 0;
	else { // is invalid. Do Validate.
		BM->barray[PBA].ValidP[index] |= (off_num);
		return 1;
	}
}

int32_t		BM_InvalidatePage(BM_T* BM, PPA_T PPA)
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

	if (BM->barray[PBA].ValidP[index] & (off_num)) { // is valid?
		BM->barray[PBA].ValidP[index] &= ~(off_num);
		BM->barray[PBA].Invalid++;
		return 1;
	}
	else  // is invalid.
		return 0;
}

/* For GC, Call this function to initialize vimctim block */
inline void BM_InitializeBlock(BM_T* BM, PBA_T PBA) {
	memset(BM->barray[PBA].ValidP, BM_INVALIDPAGE, numBITMAPB);
	BM->barray[PBA].Invalid = 0;
}
/* Initalize all Block */
inline void BM_InitializeAll(BM_T* BM) {
	for (int i=0; i<numBlock; i++){
		memset(BM->barray[i].ValidP, BM_INVALIDPAGE, numBITMAPB);
		BM->barray[i].Invalid = 0;
	}
}

// Interface Functions for 'number of invalid pages(Invalid)' change only (no Bitmap change)
inline void BM_InvalidPlus_PBA(BM_T* BM, PBA_T PBA) {
	BM->barray[PBA].Invalid++;
}
inline void BM_InvalidPlus_PPA(BM_T* BM, PPA_T PPA) {
	BM->barray[BM_PPA_TO_PBA(PPA)].Invalid++;
}
inline void BM_InvalidMinus_PBA(BM_T* BM, PBA_T PBA) {
	BM->barray[PBA].Invalid--;
}
inline void BM_InvalidMinus_PPA(BM_T* BM, PPA_T PPA) {
	BM->barray[BM_PPA_TO_PBA(PPA)].Invalid--;
}
inline void BM_InvalidZero_PBA(BM_T* BM, PBA_T PBA) {
	BM->barray[PBA].Invalid = 0;
}
inline void BM_InvalidZero_PPA(BM_T* BM, PPA_T PPA) {
	BM->barray[BM_PPA_TO_PBA(PPA)].Invalid = 0;
}
inline void BM_InvalidPPB_PBA(BM_T* BM, PBA_T PBA) {
	BM->barray[PBA].Invalid = PagePerBlock;
}
inline void BM_InvalidPPB_PPA(BM_T* BM, PPA_T PPA) {
	BM->barray[BM_PPA_TO_PBA(PPA)].Invalid = PagePerBlock;
}