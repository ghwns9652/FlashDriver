#ifndef _BM_COMMON_H_
#define _BM_COMMON_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PPB 256		/* Actually NOT THIS. REAL PPB is in "container.h" */
// container에 있는 PPB 찾아서 macro로 찾을 수 있게 바꿔놓자.
// include/settings.h 에 _PPB(256) 으로 나와있는 것 같다
#define NOP 2000000	/* Actually NOT THIS. */
#define NOB 1000	/* Actually NOT THIS. */




/* Size of ptrBlock */
/* Should be used in declaration file, and it is equal to Declaration block size */
#define BSIZE(arr)	sizeof(arr)/sizeof(Block*)


/*
 * SWAP Macros
 */

#define SWAP(a, b) {int32_t temp; temp=a; a=b; b=temp;} // 다형성을 위해 void*로 할 필요는 없을까?
#define SWAP_BLOCK(a, b)	\
	{ Block* temp; temp=a; a=b; b=temp; }
#define SWAP_PBA(a, b)		\
	{ uint32_t tempPBA; tempPBA = a->PBA; a->PBA = b->PBA; b->PBA = tempPBA; } 
#define SWAP_BIT(a, b)		\
	{ uint32_t tempbit; tempbit = a->bit; a->bit = b->bit; b->bit = tempbit; }
#define SWAP_CNT(a, b)		\
	{ uint32_t tempcnt; tempcnt = a->numValid; a->numValid = b->numValid; b->numValid = tempcnt; }
//#define SWAP_PE(a, b)		\
//	{ uint32_t tempPE; tempPE = a->PE; a->PE = b->PE; b->PE = tempPE; }






 /*
 * Error Handling
 */
 /* Error Number Indicating NO ERROR */
#define eNOERROR 0


/* error macro */
#define ERR(e)	{ printf("Error: %d\n", (int32_t)e); return e; }








/*
* Macro Definitions
*/
#define ERR_ENCODE_ERROR_CODE(base,no)      ( -1 * (((base) << 16) | no) )

/*
* Error Base Definitions
*/
#define OM_ERR_BASE                              6

/*
* Error Definitions for OM_ERR_BASE
*/
#define eHEAPUNDERFLOW_BM                        ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,0)
#define ePBASEARCH_BM							 ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,1)
#define eBADOFFSET_BM							 ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,2)
#define eBADVALIDPAGE_BM						 ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,3)
/*
#define eBADOBJECTID_OM                          ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,1)
#define eBADCATALOGOBJECT_OM                     ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,2)
#define eBADLENGTH_OM                            ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,3)
#define eBADSTART_OM                             ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,4)
#define eBADFILEID_OM                            ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,5)
#define eBADUSERBUF_OM                           ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,6)
#define eBADPAGEID_OM                            ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,7)
#define eTOOLARGESORTKEY_OM                      ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,8)
#define eCANTALLOCEXTENT_BL_OM                   ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,9)
#define NUM_ERRORS_OM_ERR_BASE                   10
#define eNOTSUPPORTED_EDUOM			             ERR_ENCODE_ERROR_CODE(OM_ERR_BASE,11)
*/


#endif // !_BM_COMMON_H_