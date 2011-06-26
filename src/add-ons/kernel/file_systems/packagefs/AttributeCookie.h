/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_COOKIE_H
#define ATTRIBUTE_COOKIE_H


#include <sys/stat.h>

#include <SupportDefs.h>


class AttributeCookie {
public:
	virtual						~AttributeCookie();

	virtual	status_t			Close();
	virtual	status_t			ReadAttribute(off_t offset, void* buffer,
									size_t* bufferSize) = 0;
	virtual	status_t			ReadAttributeStat(struct stat* st) = 0;
};


#endif	// ATTRIBUTE_COOKIE_H
