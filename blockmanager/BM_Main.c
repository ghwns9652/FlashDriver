/* BM_Main */

#include "BM_Interface.h"

int main(void)
{
    printf("********************* Bad-Block Manager *********************\n");
    printf("Start main!\n\n");
    BM_Init();



	printf("BM_Init End in MAIN!!!\n");
	
	BM_Shutdown();
	printf("BM_Shutdown End in MAIN!!\n");
	return 0;
}
