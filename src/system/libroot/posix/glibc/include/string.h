#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H

#include_next <string.h>

/* map the internal glibc interface to the public one */
#define __memcpy(to, from, size)	memcpy(to, from, size)
#define __strchrnul(string, character)	strchrnul(string, character)
#define __strnlen			strnlen

static inline char *
__strerror_r(int error, char *buffer, size_t bufferSize)
{
	if (strerror_r(error, buffer, bufferSize) != 0)
		strlcpy(buffer, "Unknown Error", bufferSize);

	return buffer;
}

static inline void *
__mempcpy(void *to, const void *from, size_t size)
{
	memcpy(to, from, size);
	return (unsigned char *)to + size;
}

#endif	/* _LIBC_STRING_H */
