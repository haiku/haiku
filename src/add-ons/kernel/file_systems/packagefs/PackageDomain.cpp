/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageDomain.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <NodeMonitor.h>

#include <AutoDeleter.h>

#include <fs/node_monitor.h>
#include <Notifications.h>

#include "DebugSupport.h"


PackageDomain::PackageDomain()
	:
	fPath(NULL),
	fDirFD(-1),
	fListener(NULL)
{
}


PackageDomain::~PackageDomain()
{
	PRINT("PackageDomain::~PackageDomain()\n");

	if (fListener != NULL) {
		remove_node_listener(fDeviceID, fNodeID, *fListener);
		delete fListener;
	}

	Package* package = fPackages.Clear(true);
	while (package != NULL) {
		Package* next = package->FileNameHashTableNext();
		package->ReleaseReference();
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
PackageDomain::Prepare(NotificationListener* listener)
{
	ObjectDeleter<NotificationListener> listenerDeleter(listener);

	fDirFD = open(fPath, O_RDONLY);
	if (fDirFD < 0) {
		ERROR("Failed to open package domain \"%s\"\n", strerror(errno));
		return errno;
	}

	struct stat st;
	if (fstat(fDirFD, &st) < 0)
		RETURN_ERROR(errno);

	fDeviceID = st.st_dev;
	fNodeID = st.st_ino;

	status_t error = add_node_listener(fDeviceID, fNodeID, B_WATCH_DIRECTORY,
		*listener);
	if (error != B_OK)
		RETURN_ERROR(error);
	fListener = listenerDeleter.Detach();

	return B_OK;
}


void
PackageDomain::AddPackage(Package* package)
{
	fPackages.Insert(package);
	package->AcquireReference();
}


void
PackageDomain::RemovePackage(Package* package)
{
	fPackages.Remove(package);
	package->ReleaseReference();
}


Package*
PackageDomain::FindPackage(const char* fileName) const
{
	return fPackages.Lookup(fileName);
}
