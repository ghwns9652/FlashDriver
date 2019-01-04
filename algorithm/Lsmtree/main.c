#include <stdio.h>

int main() {
	int a[10] = {1,2,3,4,5,6,7,8,9,10};
	int b;
	for(int i=0, b=a[i]; i < 10; i++, b=a[i]) {
		printf("%d %d\n", i,b);
	}
	return 0;
}
