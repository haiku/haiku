/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Dependency.h"

#include <stdlib.h>
#include <string.h>

#include "Version.h"


Dependency::Dependency(::Package* package)
	:
	fPackage(package),
	fFamily(NULL),
	fResolvable(NULL),
	fName(NULL),
	fVersion(NULL),
	fVersionOperator(B_PACKAGE_RESOLVABLE_OP_EQUAL)
{
}


Dependency::~Dependency()
{
	free(fName);
	delete fVersion;
}


status_t
Dependency::Init(const char* name)
{
	fName = strdup(name);
	if (fName == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
Dependency::SetVersionRequirement(BPackageResolvableOperator op,
	Version* version)
{
	fVersionOperator = op;
	fVersion = version;
}
