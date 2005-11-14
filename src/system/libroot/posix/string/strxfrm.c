
#include <string.h>

size_t
strxfrm(char *out, const char *in, size_t size)
{
	// TODO: To be implemented correctly when we have collation support.
	return strlcpy(out, in, size);
}
