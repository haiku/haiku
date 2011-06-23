/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Resolvable.h"

#include <string.h>

#include "Version.h"


Resolvable::Resolvable(Package* package)
	:
	fPackage(package),
	fName(NULL),
	fVersion(NULL)
{
}


Resolvable::~Resolvable()
{
	free(fName);
	delete fVersion;
}


status_t
Resolvable::Init(const char* name, Version* version)
{
	fVersion = version;

	fName = strdup(name);
	if (fName == NULL)
		return B_NO_MEMORY;

	return B_OK;
}
