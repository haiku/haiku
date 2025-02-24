#include <string.h>

#if __GNUC__ < 4
#define restrict
#endif

char *strcat(char *restrict dest, const char *restrict src)
{
	strcpy(dest + strlen(dest), src);
	return dest;
}
