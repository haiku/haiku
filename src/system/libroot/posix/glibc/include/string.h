#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H

#include_next <string.h>

/* map the internal glibc interface to the public one */
#define __memcpy(to, from, size)	memcpy(to, from, size)
#define __strchrnul(string, character)	strchrnul(string, character)
#define __strnlen			strnlen
#define __strerror_r			strerror_r

static inline void *
__mempcpy(void *to, const void *from, size_t size)
{
	memcpy(to, from, size);
	return (unsigned char *)to + size;
}

#endif	/* _LIBC_STRING_H */
