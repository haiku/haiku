/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef EMPTY_ATTRIBUTE_DIRECTORY_COOKIE_H
#define EMPTY_ATTRIBUTE_DIRECTORY_COOKIE_H


#include "AttributeDirectoryCookie.h"


class EmptyAttributeDirectoryCookie : public AttributeDirectoryCookie {
public:
	virtual	status_t			Close();
	virtual	status_t			Read(dev_t volumeID, ino_t nodeID,
									struct dirent* buffer, size_t bufferSize,
									uint32* _count);
	virtual	status_t			Rewind();
};


#endif	// EMPTY_ATTRIBUTE_DIRECTORY_COOKIE_H
