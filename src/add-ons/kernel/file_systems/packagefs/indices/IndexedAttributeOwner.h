/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INDEXED_ATTRIBUTE_OWNER_H
#define INDEXED_ATTRIBUTE_OWNER_H


#include <SupportDefs.h>


class IndexedAttributeOwner {
public:
	virtual						~IndexedAttributeOwner();

	virtual	void				UnsetIndexCookie(void* attributeCookie) = 0;
};


#endif	// INDEXED_ATTRIBUTE_OWNER_H
