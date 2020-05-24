/*
 * Copyright 2014 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STRINGS_H_
#define _STRINGS_H_


#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

static __inline__ int ffs(int i) { return __builtin_ffs(i); }

extern int		strcasecmp(const char *string1, const char *string2);
extern int		strncasecmp(const char *string1, const char *string2,
	size_t length);

/* legacy compatibility -- might be removed one day */
#define bcmp(a, b, length) memcmp((a), (b), (length))
#define bcopy(source, dest, length) memmove((dest), (source), (length))
#define bzero(buffer, length) memset((buffer), 0, (length))

extern char 		*index(const char *s, int c);
extern char 		*rindex(char const *s, int c);

#ifdef __cplusplus
}
#endif

#endif	/* _STRINGS_H_ */
