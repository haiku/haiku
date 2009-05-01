/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef PROPERTY_FILE_H
#define PROPERTY_FILE_H


#include <File.h>


// This is the read-only version of the PropertyFile class - the
// genprops tool contains the write-only version of it


class PropertyFile : public BFile {
	public:
		status_t SetTo(const char *directory, const char *name);

		off_t Size();
};

#endif	/* PROPERTY_FILE_H */
