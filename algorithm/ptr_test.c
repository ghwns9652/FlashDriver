#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define NOB (10)



int main(void)
{
	uint8_t numValid[NOB];
	uint32_t PE[NOB];
	for (int i=0; i<NOB; ++i){
		numValid[i] = i;
		PE[i] = NOB - i;
	}
	printf("-----------------------------------------------------------------\n");

	uint8_t **numValid_map = (uint8_t**)malloc(sizeof(uint8_t*) * NOB);
	uint32_t **PE_map = (uint32_t**)malloc(sizeof(uint32_t*) * NOB);

	for (int i=0; i<NOB; ++i){
		//numValid_map[i] = (uint8_t*)malloc(sizeof(uint8_t));
		//PE_map[i] = (uint32_t*)malloc(sizeof(uint32_t));
		numValid_map[i] = &(numValid[i]);
		PE_map[i] = &(PE[i]);
		//printf("!");
	}
	printf("\n");

	//for (int i=0; i<NOB; ++i)
		//PE_map[i] = PE+i;

	printf("numValid_map:\n\t");
	for (int i=0; i<NOB; ++i){
		printf("%d ", *numValid_map[i]);
		//printf(".. ");
	}
	printf("\n");

	printf("PE_map:\n\t");
	for (int i=0; i<NOB; ++i){
		printf("%d ", *PE_map[i]);
	}
	printf("\n");

	printf("where?\n");

	for (int i=0; i<NOB; ++i){
		//free(numValid_map[i]);
		printf("i ");
		//free(PE_map[i]);

	}
	free(numValid_map);
	free(PE_map);
	printf("Hello\n");

	return 0;
}

