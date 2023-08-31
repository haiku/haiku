#define _GNU_SOURCE
#include <string.h>
#include <stdint.h>
#include <limits.h>

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1/UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-ONES & ~(x) & HIGHS)

char *strchrnul(const char *s, int c)
{
	c = (unsigned char)c;
	if (!c) return (char *)s + strlen(s);

#if 0
	typedef size_t __attribute__((__may_alias__)) word;
	const word *w;
	for (; (uintptr_t)s % ALIGN; s++)
		if (!*s || *(unsigned char *)s == c) return (char *)s;
	size_t k = ONES * c;
	for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w^k); w++);
	s = (void *)w;
#endif
	for (; *s && *(unsigned char *)s != c; s++);
	return (char *)s;
}
