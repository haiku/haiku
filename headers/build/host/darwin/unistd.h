#ifndef _HAIKU_BUILD_COMPATIBILITY_DARWIN_UNISTD
#define _HAIKU_BUILD_COMPATIBILITY_DARWIN_UNISTD


#include_next <unistd.h>


__BEGIN_DECLS

int faccessat(int fd, const char* path, int accessMode, int flag);
int fchownat(int fd, const char* path, uid_t owner, gid_t group, int flag);
ssize_t readlinkat(int fd, const char *path, char *buffer, size_t bufferSize);
int linkat(int oldFD, const char *oldPath, int newFD, const char *newPath,
	int flag);

__END_DECLS

#endif	// _HAIKU_BUILD_COMPATIBILITY_DARWIN_UNISTD
