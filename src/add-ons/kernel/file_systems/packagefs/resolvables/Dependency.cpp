/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Dependency.h"

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>

#include "Version.h"


Dependency::Dependency(::Package* package)
	:
	fPackage(package),
	fFamily(NULL),
	fResolvable(NULL),
	fName(),
	fFileName(),
	fVersion(NULL),
	fVersionOperator(B_PACKAGE_RESOLVABLE_OP_EQUAL)
{
}


Dependency::~Dependency()
{
	delete fVersion;
}


status_t
Dependency::Init(const char* name)
{
	if (!fName.SetTo(name))
		return B_NO_MEMORY;

	// If the name contains a ':', replace it with '~' in the file name. We do
	// that so that a path containing the symlink can be used in colon-separated
	// search paths.
	if (strchr(name, ':') != NULL) {
		char* fileName = strdup(name);
		if (fileName == NULL)
			return B_NO_MEMORY;
		MemoryDeleter fileNameDeleter(fileName);

		char* remainder = fileName;
		while (char* colon = strchr(remainder, ':')) {
			*colon = '~';
			remainder = colon + 1;
		}

		if (!fFileName.SetTo(fileName))
			return B_NO_MEMORY;
	} else
		fFileName = fName;

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
		&& resolvableVersion->Compare(fVersionOperator, *fVersion);
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
			&& fVersion->Compare(*resolvableVersion) >= 0;
	}

	return true;
}
