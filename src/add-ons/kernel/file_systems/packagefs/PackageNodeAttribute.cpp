/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageNodeAttribute.h"

#include <stdlib.h>
#include <string.h>


PackageNodeAttribute::PackageNodeAttribute(uint32 type,
	const BPackageData& data)
	:
	fData(data),
	fName(NULL),
	fType(type)
{
}


PackageNodeAttribute::~PackageNodeAttribute()
{
	free(fName);
}


status_t
PackageNodeAttribute::Init(const char* name)
{
	fName = strdup(name);
	return fName != NULL ? B_OK : B_NO_MEMORY;
}
