/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_DIRECTORY_COOKIE_H
#define ATTRIBUTE_DIRECTORY_COOKIE_H


#include <dirent.h>

#include <SupportDefs.h>


class AttributeDirectoryCookie {
public:
	virtual						~AttributeDirectoryCookie();

	virtual	status_t			Close();
	virtual	status_t			Read(dev_t volumeID, ino_t nodeID,
									struct dirent* buffer, size_t bufferSize,
									uint32* _count) = 0;
	virtual	status_t			Rewind() = 0;
};


#endif	// ATTRIBUTE_DIRECTORY_COOKIE_H
