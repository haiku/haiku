/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageDomain.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "DebugSupport.h"


PackageDomain::PackageDomain()
	:
	fPath(NULL),
	fDirFD(-1)
{
}


PackageDomain::~PackageDomain()
{
	Package* package = fPackages.Clear(true);
	while (package != NULL) {
		Package* next = package->HashTableNext();
		delete package;
		package = next;
	}

	if (fDirFD >= 0)
		close(fDirFD);

	free(fPath);
}


status_t
PackageDomain::Init(const char* path)
{
	fPath = strdup(path);
	if (fPath == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	status_t error = fPackages.Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
PackageDomain::Prepare()
{
	fDirFD = open(fPath, O_RDONLY);
	if (fDirFD < 0) {
		ERROR("Failed to open package domain \"%s\"\n", strerror(errno));
		return errno;
	}

	return B_OK;
}


void
PackageDomain::AddPackage(Package* package)
{
	fPackages.Insert(package);
}


void
PackageDomain::RemovePackage(Package* package)
{
	fPackages.Remove(package);
}
