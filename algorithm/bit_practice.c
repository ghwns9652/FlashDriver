#include <stdio.h>
#include <stdint.h>

void print_num(int number){
	printf("number: %d\n", number);
}

int main(void)
{
	int number = 0x100;
	print_num(number);

	number ^= 0x1000;

	print_num(number);
}

	
