#ifndef RC_R5_COMPATIBILITY_H
#define RC_R5_COMPATIBILITY_H

#ifndef __HAIKU__

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t	strnlen(const char *string, size_t count);
extern size_t	strlcat(char *dest, const char *source, size_t length);
extern size_t	strlcpy(char *dest, const char *source, size_t length);

#ifdef __cplusplus
}	// extern "C"
#endif

#endif // !__HAIKU__

#endif	// RC_R5_COMPATIBILITY_H
