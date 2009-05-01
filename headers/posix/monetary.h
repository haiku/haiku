#ifndef _MONETARY_H_
#define _MONETARY_H_

#include <stdarg.h>

#include <LocaleBuild.h>

#ifdef __cplusplus
extern "C" {
#endif

_IMPEXP_LOCALE ssize_t strfmon(char *string, size_t maxSize, const char *format, ...);
_IMPEXP_LOCALE ssize_t vstrfmon(char *string, size_t maxSize, const char *format, va_list args);

#ifdef __cplusplus
}
#endif

#endif	/* _MONETARY_H_ */
