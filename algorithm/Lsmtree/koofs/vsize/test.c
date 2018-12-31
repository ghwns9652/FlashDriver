#include <stdio.h>
#include <string.h>

int vsizekey_cmp (char *key1, char *key2) {
	int sz_key1, sz_key2, sz;
	if (key1 == NULL) {
		if (key2 == NULL) {
			return 0;
		} else {
			return 1;
		}
	} else if (key2 == NULL) {
		return -1;
	}
	sz_key1 = strlen(key1);
	sz_key2 = strlen(key2);
	sz = sz_key1 > sz_key2 ? sz_key1 : sz_key2;
	
	return memcmp(key1, key2, sz);
}

int main() {
	char *str1 = "4/a/b/c/d/e";	
	char *str2 = "3/a/b/c/e";	

	printf("%s %s: %d\n", str1, str2,vsizekey_cmp(str1,str2));
}
