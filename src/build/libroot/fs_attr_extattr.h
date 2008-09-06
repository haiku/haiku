/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_ATTR_EXTATTR_H
#define FS_ATTR_EXTATTR_H

/*!	Included by fs_attr_untyped.cpp. Interfaces with FreeBSD extattr support.
*/


#include <string.h>
#include <sys/extattr.h>


// the namespace all attributes live in
static const char* kAttributeNamespace = "haiku.";
static const int kAttributeNamespaceLen = 6;


static ssize_t
list_attributes(int fd, const char* path, char* buffer, size_t bufferSize)
{
	ssize_t bytesRead;
	if (fd >= 0) {
		bytesRead = extattr_list_fd(fd, EXTATTR_NAMESPACE_USER, buffer,
			bufferSize);
	} else {
		bytesRead = extattr_list_link(path, EXTATTR_NAMESPACE_USER, buffer,
			bufferSize);
	}

	if (bytesRead <= 0)
		return bytesRead;

	// The listing is in a different format than expected by the caller. Here
	// we get a sequence of (<namelen>, <unterminated name>) pairs, but expected
	// is a sequence of null-terminated names. Let's convert it.
	int index = *buffer;
	memmove(buffer, buffer + 1, bytesRead - 1);

	while (index < bytesRead - 1) {
		int len = buffer[index];
		buffer[index] = '\0';
		index += len + 1;
	}

	buffer[bytesRead - 1] = '\0';

	return bytesRead;
}


static ssize_t
get_attribute(int fd, const char* path, const char* attribute, void* buffer,
	size_t bufferSize)
{
	if (fd >= 0) {
		return extattr_get_fd(fd, EXTATTR_NAMESPACE_USER, attribute, buffer,
			bufferSize);
	}
	return extattr_get_link(path, EXTATTR_NAMESPACE_USER, attribute, buffer,
		bufferSize);
}


static int
set_attribute(int fd, const char* path, const char* attribute,
	const void* buffer, size_t bufferSize)
{
 	if (fd >= 0) {
		return extattr_set_fd(fd, EXTATTR_NAMESPACE_USER, attribute, buffer,
			bufferSize);
	}
	return extattr_set_link(path, EXTATTR_NAMESPACE_USER, attribute, buffer,
			bufferSize);
}


static int
remove_attribute(int fd, const char* path, const char* attribute)
{
	if (fd >= 0)
		return extattr_delete_fd(fd, EXTATTR_NAMESPACE_USER, attribute);
	return extattr_delete_link(path, EXTATTR_NAMESPACE_USER, attribute);
}


#endif	// FS_ATTR_EXTATTR_H
