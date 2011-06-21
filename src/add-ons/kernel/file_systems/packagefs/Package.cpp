/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Package.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <util/AutoLock.h>

#include "DebugSupport.h"
#include "PackageDomain.h"


Package::Package(PackageDomain* domain, dev_t deviceID, ino_t nodeID)
	:
	fDomain(domain),
	fFileName(NULL),
	fFD(-1),
	fOpenCount(0),
	fNodeID(nodeID),
	fDeviceID(deviceID)
{
	mutex_init(&fLock, "packagefs package");
}


Package::~Package()
{
	while (PackageNode* node = fNodes.RemoveHead())
		node->ReleaseReference();

	free(fFileName);

	mutex_destroy(&fLock);
}


status_t
Package::Init(const char* fileName)
{
	fFileName = strdup(fileName);
	if (fFileName == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}


void
Package::AddNode(PackageNode* node)
{
	fNodes.Add(node);
	node->AcquireReference();
}


int
Package::Open()
{
	MutexLocker locker(fLock);
	if (fOpenCount > 0) {
		fOpenCount++;
		return fFD;
	}

	// open the file
	fFD = openat(fDomain->DirectoryFD(), fFileName, O_RDONLY);
	if (fFD < 0) {
		ERROR("Failed to open package file \"%s\"\n", fFileName);
		return errno;
	}

	// stat it to verify that it's still the same file
	struct stat st;
	if (fstat(fFD, &st) < 0) {
		ERROR("Failed to stat package file \"%s\"\n", fFileName);
		close(fFD);
		fFD = -1;
		return errno;
	}

	if (st.st_dev != fDeviceID || st.st_ino != fNodeID) {
		close(fFD);
		fFD = -1;
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	}

	fOpenCount = 1;
	return fFD;
}


void
Package::Close()
{
	MutexLocker locker(fLock);
	if (fOpenCount == 0) {
		ERROR("Package open count already 0!\n");
		return;
	}

	if (--fOpenCount == 0) {
		close(fFD);
		fFD = -1;
	}
}
