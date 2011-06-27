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


bool
Dependency::ResolvableVersionMatches(Version* resolvableVersion) const
{
	if (fVersion == NULL)
		return true;

	return resolvableVersion != NULL
		&& fVersion->Compare(fVersionOperator, *resolvableVersion);
}


bool
Dependency::ResolvableCompatibleVersionMatches(Version* resolvableVersion) const
{
	if (fVersion == NULL)
		return true;

	// Only compare the versions, if the operator indicates that our version is
	// some kind of minimum required version. Only in that case the resolvable's
	// actual version is required to be greater (or equal) and the backwards
	// compatibility check makes sense.
	if (fVersionOperator == B_PACKAGE_RESOLVABLE_OP_GREATER_EQUAL
		|| fVersionOperator == B_PACKAGE_RESOLVABLE_OP_GREATER) {
		return resolvableVersion != NULL
			&& fVersion->Compare(fVersionOperator, *resolvableVersion);
	}

	return true;
}
