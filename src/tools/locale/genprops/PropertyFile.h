/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef PROPERTY_FILE_H
#define PROPERTY_FILE_H


#include <File.h>


// This is the write-only version of the PropertyFile class - the library
// contains a read-only version of it


class PropertyFile : public BFile {
	public:
		status_t SetTo(const char *directory, const char *name);

		off_t Size();
		ssize_t WritePadding(size_t length);
};

#endif	/* PROPERTY_FILE_H */
