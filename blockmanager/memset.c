#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(void)
{
	uint64_t ValidP[4];

	for (int i=0; i<4; i++) {
		ValidP[i] = 0xcccccccccccccccc;
		printf("Valid[%d]: 0x%.16lx\n", i, ValidP[i]);
	}

	memset(ValidP, 0xff, sizeof(uint64_t)*4);
	for (int i=0; i<4; i++)
		printf("Valid[%d]: 0x%.16lx\n", i, ValidP[i]);
}

	
