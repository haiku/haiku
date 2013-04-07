/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Volume.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>

#include <AutoDeleter.h>

#include "DebugSupport.h"


Volume::Volume()
	:
	BHandler(),
	fPath(),
	fMountType(PACKAGE_FS_MOUNT_TYPE_CUSTOM),
	fRootDirectoryRef(),
	fPackagesDirectoryRef(),
	fRoot(NULL),
	fPackagesByFileName(),
	fPackagesByNodeRef()
{
}


Volume::~Volume()
{
	fPackagesByFileName.Clear();

	Package* package = fPackagesByNodeRef.Clear(true);
	while (package != NULL) {
		Package* next = package->NodeRefHashTableNext();
		delete package;
		package = next;
	}
}


status_t
Volume::Init(const node_ref& rootDirectoryRef, node_ref& _packageRootRef)
{
	if (fPackagesByFileName.Init() != B_OK || fPackagesByNodeRef.Init() != B_OK)
		RETURN_ERROR(B_NO_MEMORY);

	fRootDirectoryRef = rootDirectoryRef;

	// open the root directory
	BDirectory directory;
	status_t error = directory.SetTo(&fRootDirectoryRef);
	if (error != B_OK) {
		ERROR("Volume::Init(): failed to open root directory: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	// get the directory path
	BEntry entry;
	error = directory.GetEntry(&entry);

	BPath path;
	if (error == B_OK)
		error = entry.GetPath(&path);

	if (error != B_OK) {
		ERROR("Volume::Init(): failed to get root directory path: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	fPath = path.Path();
	if (fPath.IsEmpty())
		RETURN_ERROR(B_NO_MEMORY);

	// get a volume info from the FS
	int fd = directory.Dup();
	if (fd < 0) {
		ERROR("Volume::Init(): failed to get root directory FD: %s\n",
			strerror(fd));
		RETURN_ERROR(fd);
	}
	FileDescriptorCloser fdCloser(fd);

	PackageFSVolumeInfo info;
	if (ioctl(fd, PACKAGE_FS_OPERATION_GET_VOLUME_INFO, &info, sizeof(info))
			!= 0) {
		ERROR("Volume::Init(): failed to get volume info: %s\n",
			strerror(errno));
		RETURN_ERROR(errno);
	}

	fMountType = info.mountType;
	fPackagesDirectoryRef.device = info.packagesDeviceID;
	fPackagesDirectoryRef.node = info.packagesDirectoryID;

	// read in all packages in the directory
	error = _ReadPackagesDirectory();
	if (error != B_OK)
		RETURN_ERROR(error);

	_GetActivePackages(fd);

	_packageRootRef.device = info.rootDeviceID;
	_packageRootRef.node = info.rootDirectoryID;

	return B_OK;
}

void
Volume::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				break;

			switch (opcode) {
				case B_ENTRY_CREATED:
					_HandleEntryCreatedOrRemoved(message, true);
					break;
				case B_ENTRY_REMOVED:
					_HandleEntryCreatedOrRemoved(message, false);
					break;
				case B_ENTRY_MOVED:
					_HandleEntryMoved(message);
					break;
				default:
					break;
			}
			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


int
Volume::OpenRootDirectory() const
{
	BDirectory directory;
	status_t error = directory.SetTo(&fRootDirectoryRef);
	if (error != B_OK) {
		ERROR("Volume::OpenRootDirectory(): failed to open root directory: "
			"%s\n", strerror(error));
		RETURN_ERROR(error);
	}

	return directory.Dup();
}


void
Volume::_HandleEntryCreatedOrRemoved(const BMessage* message, bool created)
{
	// only moves to or from our packages directory are interesting
	int32 deviceID;
	int64 directoryID;
	const char* name;
	if (message->FindInt32("device", &deviceID) != B_OK
		|| message->FindInt64("directory", &directoryID) != B_OK
		|| message->FindString("name", &name) != B_OK
		|| node_ref(deviceID, directoryID) != fPackagesDirectoryRef) {
		return;
	}

	if (created)
		_PackagesEntryCreated(name);
	else
		_PackagesEntryRemoved(name);
}


void
Volume::_HandleEntryMoved(const BMessage* message)
{
	int32 deviceID;
	int64 fromDirectoryID;
	int64 toDirectoryID;
	const char* fromName;
	const char* toName;
	if (message->FindInt32("device", &deviceID) != B_OK
		|| message->FindInt64("from directory", &fromDirectoryID) != B_OK
		|| message->FindInt64("to directory", &toDirectoryID) != B_OK
		|| message->FindString("from name", &fromName) != B_OK
		|| message->FindString("name", &toName) != B_OK
		|| deviceID != fPackagesDirectoryRef.device
		|| (fromDirectoryID != fPackagesDirectoryRef.node
			&& toDirectoryID != fPackagesDirectoryRef.node)) {
		return;
	}

	if (fromDirectoryID == fPackagesDirectoryRef.node)
		_PackagesEntryRemoved(fromName);
	if (toDirectoryID == fPackagesDirectoryRef.node)
		_PackagesEntryCreated(toName);
}


void
Volume::_PackagesEntryCreated(const char* name)
{
INFORM("Volume::_PackagesEntryCreated(\"%s\")\n", name);
	entry_ref entry;
	entry.device = fPackagesDirectoryRef.device;
	entry.directory = fPackagesDirectoryRef.node;
	status_t error = entry.set_name(name);
	if (error != B_OK) {
		ERROR("out of memory");
		return;
	}

	Package* package = new(std::nothrow) Package;
	if (package == NULL) {
		ERROR("out of memory");
		return;
	}
	ObjectDeleter<Package> packageDeleter(package);

	error = package->Init(entry);
	if (error != B_OK) {
		ERROR("failed to init package for file \"%s\"\n", name);
		return;
	}

	fPackagesByFileName.Insert(package);
	fPackagesByNodeRef.Insert(package);
	packageDeleter.Detach();

	// activate package
// TODO: Don't do that here!
	size_t nameLength = strlen(package->FileName());
	size_t requestSize = sizeof(PackageFSActivationChangeRequest) + nameLength;
	PackageFSActivationChangeRequest* request
		= (PackageFSActivationChangeRequest*)malloc(requestSize);
	if (request == NULL) {
		ERROR("out of memory");
		return;
	}
	MemoryDeleter requestDeleter(request);

	request->itemCount = 1;
	PackageFSActivationChangeItem& item = request->items[0];
	item.type = PACKAGE_FS_ACTIVATE_PACKAGE;

	item.packageDeviceID = package->NodeRef().device;
	item.packageNodeID = package->NodeRef().node;

	item.nameLength = nameLength;
	item.parentDeviceID = fPackagesDirectoryRef.device;
	item.parentDirectoryID = fPackagesDirectoryRef.node;
	strcpy(item.name, package->FileName());

	int fd = OpenRootDirectory();
	if (fd < 0) {
		ERROR("Volume::_PackagesEntryCreated(): failed to open root directory: "
			"%s", strerror(fd));
		return;
	}
	FileDescriptorCloser fdCloser(fd);

	if (ioctl(fd, PACKAGE_FS_OPERATION_CHANGE_ACTIVATION, request, requestSize)
			!= 0) {
		ERROR("Volume::_PackagesEntryCreated(): activate packages: %s\n",
			strerror(errno));
		return;
	}

	package->SetActive(true);
}


void
Volume::_PackagesEntryRemoved(const char* name)
{
INFORM("Volume::_PackagesEntryRemoved(\"%s\")\n", name);
	Package* package = fPackagesByFileName.Lookup(name);
	if (package == NULL)
		return;

	if (package->IsActive()) {
		// deactivate the package
// TODO: Don't do that here!
		size_t nameLength = strlen(package->FileName());
		size_t requestSize = sizeof(PackageFSActivationChangeRequest)
			+ nameLength;
		PackageFSActivationChangeRequest* request
			= (PackageFSActivationChangeRequest*)malloc(requestSize);
		if (request == NULL) {
			ERROR("out of memory");
			return;
		}
		MemoryDeleter requestDeleter(request);

		request->itemCount = 1;
		PackageFSActivationChangeItem& item = request->items[0];
		item.type = PACKAGE_FS_DEACTIVATE_PACKAGE;

		item.packageDeviceID = package->NodeRef().device;
		item.packageNodeID = package->NodeRef().node;

		item.nameLength = nameLength;
		item.parentDeviceID = fPackagesDirectoryRef.device;
		item.parentDirectoryID = fPackagesDirectoryRef.node;
		strcpy(item.name, package->FileName());

		int fd = OpenRootDirectory();
		if (fd < 0) {
			ERROR("Volume::_PackagesEntryRemoved(): failed to open root "
				"directory: %s", strerror(fd));
			return;
		}
		FileDescriptorCloser fdCloser(fd);

		if (ioctl(fd, PACKAGE_FS_OPERATION_CHANGE_ACTIVATION, request,
				requestSize) != 0) {
			ERROR("Volume::_PackagesEntryRemoved(): activate packages: %s\n",
				strerror(errno));
			return;
		}
	}

	fPackagesByFileName.Remove(package);
	fPackagesByNodeRef.Remove(package);
	delete package;
}


status_t
Volume::_ReadPackagesDirectory()
{
	BDirectory directory;
	status_t error = directory.SetTo(&fPackagesDirectoryRef);
	if (error != B_OK) {
		ERROR("Volume::_ReadPackagesDirectory(): open packages directory: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	entry_ref entry;
	while (directory.GetNextRef(&entry) == B_OK) {
		Package* package = new(std::nothrow) Package;
		if (package == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		ObjectDeleter<Package> packageDeleter(package);

		status_t error = package->Init(entry);
		if (error == B_OK) {
			fPackagesByFileName.Insert(package);
			fPackagesByNodeRef.Insert(package);
			packageDeleter.Detach();
		}
	}

	return B_OK;
}


status_t
Volume::_GetActivePackages(int fd)
{
	uint32 maxPackageCount = 16 * 1024;
	PackageFSGetPackageInfosRequest* request = NULL;
	MemoryDeleter requestDeleter;
	size_t bufferSize;
	for (;;) {
		bufferSize = sizeof(PackageFSGetPackageInfosRequest)
			+ (maxPackageCount - 1) * sizeof(PackageFSPackageInfo);
		request = (PackageFSGetPackageInfosRequest*)malloc(bufferSize);
		if (request == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		requestDeleter.SetTo(request);

		if (ioctl(fd, PACKAGE_FS_OPERATION_GET_PACKAGE_INFOS, request,
				bufferSize) != 0) {
			ERROR("Volume::_GetActivePackages(): failed to get active package "
				"info from package FS: %s\n", strerror(errno));
			RETURN_ERROR(errno);
		}

		if (request->packageCount <= maxPackageCount)
			break;

		maxPackageCount = request->packageCount;
		requestDeleter.Unset();
	}

	// mark the returned packages active
	for (uint32 i = 0; i < request->packageCount; i++) {
		Package* package = fPackagesByNodeRef.Lookup(
			node_ref(request->infos[i].packageDeviceID,
				request->infos[i].packageNodeID));
		if (package == NULL) {
			WARN("active package (dev: %" B_PRIdDEV ", node: %" B_PRIdINO ") "
				"not found in package directory\n",
				request->infos[i].packageDeviceID,
				request->infos[i].packageNodeID);
// TODO: Deactivate the package right away?
			continue;
		}

		package->SetActive(true);
INFORM("active package: \"%s\"\n", package->FileName().String());
	}

for (PackageNodeRefHashTable::Iterator it = fPackagesByNodeRef.GetIterator();
	it.HasNext();) {
	Package* package = it.Next();
	if (!package->IsActive())
		INFORM("inactive package: \"%s\"\n", package->FileName().String());
}

			PackageNodeRefHashTable fPackagesByNodeRef;
// INFORM("%" B_PRIu32 " active packages:\n", request->packageCount);
// for (uint32 i = 0; i < request->packageCount; i++) {
// 	INFORM("  dev: %" B_PRIdDEV ", node: %" B_PRIdINO "\n",
// 		request->infos[i].packageDeviceID, request->infos[i].packageNodeID);
// }

	return B_OK;
}
