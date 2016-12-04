#include <string.h>

int strcmp(const char* a, const char* b) {
	int lena = strlen(a);
	int lenb = strlen(b);

	if (lena == lenb) {
		return memcmp(a, b, lena);
	} else if (lena < lenb) {
		return -1;
	} else if (lena > lenb) {
		return 1;
	}
}

char *strcpy(char *dest, const char *src) {
	return (char *)memcpy(dest, src, strlen(src));
}