/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_ATTR_XATTR_H
#define FS_ATTR_XATTR_H

/*!	Included by fs_attr_untyped.cpp. Interfaces with Linux xattr support.
*/


#include <sys/xattr.h>


// the namespace all attributes live in
static const char* kAttributeNamespace = "user.haiku.";
static const int kAttributeNamespaceLen = 11;


static ssize_t
list_attributes(int fd, const char* path, char* buffer, size_t bufferSize)
{
	if (fd >= 0)
		return flistxattr(fd, buffer, bufferSize);
	return llistxattr(path, buffer, bufferSize);
}


static ssize_t
get_attribute(int fd, const char* path, const char* attribute, void* buffer,
	size_t bufferSize)
{
	if (fd >= 0)
		return fgetxattr(fd, attribute, buffer, bufferSize);
	return lgetxattr(path, attribute, buffer, bufferSize);
}


static int
set_attribute(int fd, const char* path, const char* attribute,
	const void* buffer, size_t bufferSize)
{
	if (fd >= 0)
		return fsetxattr(fd, attribute, buffer, bufferSize, 0);
	return lsetxattr(path, attribute, buffer, bufferSize, 0);
}


static int
remove_attribute(int fd, const char* path, const char* attribute)
{
	if (fd >= 0)
		return fremovexattr(fd, attribute);
	return lremovexattr(path, attribute);
}


#endif	// FS_ATTR_XATTR_H
