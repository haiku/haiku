#ifndef	_STDIO_EXT_H

# include <stdio-common/stdio_ext.h>

extern int __fsetlocking_internal (FILE *__fp, int __type) attribute_hidden;

#ifndef NOT_IN_libc
# define __fsetlocking(fp, type) INTUSE(__fsetlocking) (fp, type)
#endif

#endif
