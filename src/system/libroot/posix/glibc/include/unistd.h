#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H

#include_next <unistd.h>

/* map the internal glibc interface to the public one */
#define __isatty(fd) \
	isatty(fd)

#define __close(fd) \
	close(fd)

#define __read(fd, buffer, size) \
	read(fd, buffer, size)

#define __write(fd, buffer, size) \
	write(fd, buffer, size)

#define __lseek(fd, pos, whence) \
	lseek(fd, pos, whence)

#define __unlink(buf) \
	unlink(buf)

#define __getcwd(buf, size) \
	getcwd(buf, size)

#endif	/* _LIBC_UNISTD_H */
