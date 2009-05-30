/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Sebastian Nozzi.
 *
 * Distributed under the terms of the MIT license.
 */
#ifndef _ADD_ATTR_H
#define _ADD_ATTR_H


#include <SupportDefs.h>


status_t addAttr(const char *file, type_code attrType, const char *attrName,
	const char *attrValue, size_t length, bool resolveLinks);

#endif	/* _ADD_ATTR_H */
