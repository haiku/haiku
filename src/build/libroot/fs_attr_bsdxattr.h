/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_ATTR_BSDXATTR_H
#define FS_ATTR_BSDXATTR_H

/*!	Included by fs_attr_untyped.cpp. Interfaces with BSD xattr support.
*/


#include <sys/xattr.h>


// the namespace all attributes live in
static const char* kAttributeNamespace = "user.haiku.";
static const int kAttributeNamespaceLen = 11;


static ssize_t
list_attributes(int fd, const char* path, char* buffer, size_t bufferSize)
{
	if (fd >= 0)
		return flistxattr(fd, buffer, bufferSize, 0);
	return listxattr(path, buffer, bufferSize, XATTR_NOFOLLOW);
}


static ssize_t
get_attribute(int fd, const char* path, const char* attribute, void* buffer,
	size_t bufferSize)
{
	if (fd >= 0)
		return fgetxattr(fd, attribute, buffer, bufferSize, 0, 0);
	return getxattr(path, attribute, buffer, bufferSize, 0, XATTR_NOFOLLOW);
}


static int
set_attribute(int fd, const char* path, const char* attribute,
	const void* buffer, size_t bufferSize)
{
	if (fd >= 0)
		return fsetxattr(fd, attribute, buffer, bufferSize, 0, 0);
	return setxattr(path, attribute, buffer, bufferSize, 0, XATTR_NOFOLLOW);
}


static int
remove_attribute(int fd, const char* path, const char* attribute)
{
	if (fd >= 0)
		return fremovexattr(fd, attribute, 0);
	return removexattr(path, attribute, XATTR_NOFOLLOW);
}


#endif	// FS_ATTR_BSDXATTR_H
