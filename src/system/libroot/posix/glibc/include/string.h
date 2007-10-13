#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H

#include_next <string.h>

/* map the internal glibc interface to the public one */
#define __memcpy(to, from, size)	memcpy(to, from, size)
#define __strchrnul(string, character)	strchrnul(string, character)
#define __strnlen			strnlen
#define __strdup			strdup
#define __strndup			strndup
#define __stpcpy			stpcpy
#define __strtok_r			strtok_r
#define __strcasecmp			strcasecmp

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

#undef strdupa
#define strdupa(s)                                                           \
  (__extension__                                                              \
    ({                                                                        \
      __const char *__old = (s);                                              \
      size_t __len = strlen (__old) + 1;                                      \
      char *__new = (char *) __builtin_alloca (__len);                        \
      (char *) memcpy (__new, __old, __len);                                  \
    }))

/* Alternative version which doesn't pollute glibc's namespace.  */
#undef strndupa
#define strndupa(s, n)                                                        \
  (__extension__                                                              \
    ({                                                                        \
      __const char *__old = (s);                                              \
      size_t __len = __strnlen (__old, (n));                                  \
      char *__new = (char *) __builtin_alloca (__len + 1);                    \
      __new[__len] = '\0';                                                    \
      (char *) memcpy (__new, __old, __len);                                  \
    }))

#endif	/* _LIBC_STRING_H */
