#ifndef _MONETARY_H_
#define _MONETARY_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t strfmon(char *string, size_t maxSize, const char *format, ...);
ssize_t vstrfmon(char *string, size_t maxSize, const char *format, va_list args);

#ifdef __cplusplus
}
#endif

#endif	/* _MONETARY_H_ */
