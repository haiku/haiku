/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * The GNU/Linux xattr interface. Actual xattrs are identity-mapped into the
 * Haiku attribute namespace with type B_XATTR_TYPE. Haiku attributes are mapped
 * into a user xattr namespace, the attribute types encoded in the names.
 */
#ifndef _GNU_SYS_CDEFS_H
#define _GNU_SYS_CDEFS_H


#include <sys/cdefs.h>
#include <sys/types.h>


/* constants for [l,f]setxattr() */
#define XATTR_CREATE	1	/* fail if attribute exists */
#define XATTR_REPLACE	2	/* fail if attribute doesn't exist yet */


__BEGIN_DECLS


ssize_t	getxattr(const char* path, const char* attribute, void* buffer,
			size_t size);
ssize_t	lgetxattr(const char* path, const char* attribute, void* buffer,
			size_t size);
ssize_t	fgetxattr(int fd, const char* attribute, void* buffer, size_t size);

int		setxattr(const char* path, const char* attribute, const void* buffer,
			size_t size, int flags);
int		lsetxattr(const char* path, const char* attribute, const void* buffer,
			size_t size, int flags);
int		fsetxattr(int fd, const char* attribute, const void* buffer,
			size_t size, int flags);

int		removexattr (const char* path, const char* attribute);
int		lremovexattr (const char* path, const char* attribute);
int		fremovexattr (int fd, const char* attribute);

ssize_t	listxattr(const char* path, char* buffer, size_t size);
ssize_t	llistxattr(const char* path, char* buffer, size_t size);
ssize_t	flistxattr(int fd, char* buffer, size_t size);


__END_DECLS


#endif	/* _GNU_SYS_CDEFS_H */
