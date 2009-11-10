/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "PropertyFile.h"
#include "UnicodeProperties.h"

#include <Path.h>
#include <FindDirectory.h>


status_t
PropertyFile::SetTo(const char *directory, const char *name)
{
	BPath path;
	status_t status = find_directory(B_BEOS_DATA_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append(directory);
	path.Append(name);
	status = BFile::SetTo(path.Path(), B_READ_ONLY);
	if (status < B_OK)
		return status;

	UnicodePropertiesHeader header;
	ssize_t bytes = Read(&header, sizeof(header));
	if (bytes < (ssize_t)sizeof(header)
		|| header.size != (uint8)sizeof(header)
		|| header.isBigEndian != B_HOST_IS_BENDIAN
		|| header.format != PROPERTIES_FORMAT)
		return B_BAD_DATA;

	return B_OK;
}


off_t 
PropertyFile::Size()
{
	off_t size;
	if (GetSize(&size) < B_OK)
		return 0;

	return size - sizeof(UnicodePropertiesHeader);
}

