#ifndef _FSSH_STDIO_H_
#define _FSSH_STDIO_H_

#include <stdarg.h>

#include "fssh_defs.h"


#ifdef FSSH_EOF
#	undef FSSH_EOF
#endif
#define FSSH_EOF -1


#ifdef __cplusplus
extern "C" {
#endif

/* file operations */
extern int		fssh_remove(const char *name);
extern int		fssh_rename(const char *from, const char *to);

/* formatted I/O */
extern int		fssh_sprintf(char *string, char const *format, ...) 
						__attribute__ ((format (__printf__, 2, 3)));
extern int		fssh_snprintf(char *string, fssh_size_t size,
						char const *format, ...)
						__attribute__ ((format (__printf__, 3, 4)));
extern int		fssh_vsprintf(char *string, char const *format, va_list ap);
extern int		fssh_vsnprintf(char *string, fssh_size_t size,
						char const *format, va_list ap);

extern int		fssh_sscanf(char const *str, char const *format, ...);
extern int		fssh_vsscanf(char const *str, char const *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif	/* _FSSH_STDIO_H_ */
