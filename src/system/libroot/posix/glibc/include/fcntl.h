#ifndef _FCNTL_H

#include_next <fcntl.h>
#include <features.h>

/* Now define the internal interfaces.  */
extern int __open64 (__const char *__file, int __oflag, ...);
libc_hidden_proto (__open64)
extern int __libc_open64 (const char *file, int oflag, ...);
extern int __libc_open (const char *file, int oflag, ...);
libc_hidden_proto (__libc_open)
extern int __libc_creat (const char *file, mode_t mode);
extern int __libc_fcntl (int fd, int cmd, ...);
libc_hidden_proto (__libc_fcntl)
extern int __open (__const char *__file, int __oflag, ...);
libc_hidden_proto (__open)

#endif
