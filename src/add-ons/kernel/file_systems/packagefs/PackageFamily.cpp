/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageFamily.h"

#include <stdlib.h>
#include <string.h>

#include "DebugSupport.h"
#include "Version.h"


PackageFamily::PackageFamily()
	:
	fName(NULL)
{
}


PackageFamily::~PackageFamily()
{
	while (Package* package = fPackages.RemoveHead())
		package->SetFamily(NULL);

	free(fName);
}


status_t
PackageFamily::Init(Package* package)
{
	// compute the allocation size needed for the versioned name
	size_t nameLength = strlen(package->Name());
	size_t size = nameLength + 1;

	Version* version = package->Version();
	if (version != NULL) {
		size += 1 + version->ToString(NULL, 0);
			// + 1 for the '-'
	}

	// allocate the name and compose it
	fName = (char*)malloc(size);
	if (fName == NULL)
		return B_NO_MEMORY;

	memcpy(fName, package->Name(), nameLength + 1);
	if (version != NULL) {
		fName[nameLength] = '-';
		version->ToString(fName + nameLength + 1, size - nameLength - 1);
	}

	// add the package
	AddPackage(package);

	return B_OK;
}


void
PackageFamily::AddPackage(Package* package)
{
	fPackages.Add(package);
	package->SetFamily(this);
}


void
PackageFamily::RemovePackage(Package* package)
{
	ASSERT(package->Family() == this);

	package->SetFamily(NULL);
	fPackages.Remove(package);
}
