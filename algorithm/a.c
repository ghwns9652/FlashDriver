#include <stdio.h>

#include <stdint.h>
int main(void)
{
	unsigned char a = 10;

	char offset = 1;

	a = a >> offset;

	printf("After right shift: %d\n", a);

	a = 10;
	printf("a = 10\n");
	a = a << 8 - (offset+1);

	printf("After left shift: %d\n", a);

	a = a >> 7;
	printf("After left-right shift: %d\n", a);
	printf("Is that 1?\n");

	printf("Lets check uint64_t bit..\n");


	uint64_t bit = 4503668346847230;
	printf("bit: %lu\n", bit);	
	offset = 64+53;
	offset = offset % 64;

	bit = bit << 64 - (offset+1);
	bit = bit >> 63;

	// Now, bit means offset bit (1 or 0);

	printf("offset bit(1 or 0): %lu\n", bit);
	
	return 0;
}

