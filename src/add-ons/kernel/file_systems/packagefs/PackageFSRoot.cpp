/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageFSRoot.h"

#include <AutoDeleter.h>

#include <vfs.h>

#include "DebugSupport.h"


mutex PackageFSRoot::sRootListLock = MUTEX_INITIALIZER("packagefs root list");
PackageFSRoot::RootList PackageFSRoot::sRootList;


PackageFSRoot::PackageFSRoot(dev_t deviceID, ino_t nodeID)
	:
	fDeviceID(deviceID),
	fNodeID(nodeID),
	fSystemVolume(NULL)
{
	rw_lock_init(&fLock, "packagefs root");
}


PackageFSRoot::~PackageFSRoot()
{
	rw_lock_destroy(&fLock);
}


/*static*/ status_t
PackageFSRoot::GlobalInit()
{
	return B_OK;
}


/*static*/ void
PackageFSRoot::GlobalUninit()
{
}


status_t
PackageFSRoot::Init()
{
	status_t error = fPackageFamilies.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}


/*static*/ status_t
PackageFSRoot::RegisterVolume(Volume* volume)
{
	// Unless the volume is custom mounted, we stat the supposed root directory.
	// Get the volume mount point relative path to the root directory depending
	// on the mount type.
	const char* relativeRootPath = NULL;

	switch (volume->MountType()) {
		case MOUNT_TYPE_SYSTEM:
		case MOUNT_TYPE_COMMON:
			relativeRootPath = "..";
			break;
		case MOUNT_TYPE_HOME:
			relativeRootPath = "../..";
			break;
		case MOUNT_TYPE_CUSTOM:
		default:
			break;
	}

	if (relativeRootPath != NULL) {
		struct vnode* vnode;
		status_t error = vfs_entry_ref_to_vnode(volume->MountPointDeviceID(),
			volume->MountPointNodeID(), relativeRootPath, &vnode);
		if (error != B_OK) {
			dprintf("packagefs: Failed to get root directory \"%s\": %s\n",
				relativeRootPath, strerror(error));
			RETURN_ERROR(error);
		}
		CObjectDeleter<struct vnode> vnodePutter(vnode, &vfs_put_vnode);

		// stat it
		struct stat st;
		error = vfs_stat_vnode(vnode, &st);
		if (error != B_OK) {
			dprintf("packagefs: Failed to stat root directory \"%s\": %s\n",
				relativeRootPath, strerror(error));
			RETURN_ERROR(error);
		}

		// get/create the root
		PackageFSRoot* root;
		error = PackageFSRoot::_GetOrCreateRoot(st.st_dev, st.st_ino, root);
		if (error != B_OK)
			RETURN_ERROR(error);

		// add the volume
		error = root->_AddVolume(volume);
		if (error != B_OK) {
			_PutRoot(root);
			RETURN_ERROR(error);
		}

		return B_OK;
	}

	// custom mount -- always create a new root
	PackageFSRoot* root = new(std::nothrow) PackageFSRoot(-1, 0);
	if (root == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PackageFSRoot> rootDeleter(root);

	status_t error = root->Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the volume
	error = root->_AddVolume(volume);
	if (error != B_OK) {
		_PutRoot(root);
		RETURN_ERROR(error);
	}

	// We don't add the root to the list.
	rootDeleter.Detach();
	return B_OK;
}


void
PackageFSRoot::UnregisterVolume(Volume* volume)
{
	_RemoveVolume(volume);
	_PutRoot(this);
}


status_t
PackageFSRoot::AddPackage(Package* package)
{
	// Create a package family -- there might already be one, but since that's
	// unlikely, we don't bother to check and recheck later.
	PackageFamily* packageFamily = new(std::nothrow) PackageFamily;
	if (packageFamily == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PackageFamily> packageFamilyDeleter(packageFamily);

	status_t error = packageFamily->Init(package);
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the family
	PackageFSRootWriteLocker writeLocker(this);
	if (PackageFamily* otherPackageFamily
			= fPackageFamilies.Lookup(packageFamily->Name())) {
		packageFamily->RemovePackage(package);
		packageFamily = otherPackageFamily;
		packageFamily->AddPackage(package);
	} else
		fPackageFamilies.Insert(packageFamilyDeleter.Detach());

// TODO:...

	return B_OK;
}


void
PackageFSRoot::RemovePackage(Package* package)
{
	PackageFSRootWriteLocker writeLocker(this);

	PackageFamily* packageFamily = package->Family();
	if (packageFamily == NULL)
		return;

	packageFamily->RemovePackage(package);

	if (packageFamily->IsEmpty()) {
		fPackageFamilies.Remove(packageFamily);
		delete packageFamily;
	}

// TODO:...
}


Volume*
PackageFSRoot::SystemVolume() const
{
	PackageFSRootReadLocker readLocker(this);
	return fSystemVolume;
}


status_t
PackageFSRoot::_AddVolume(Volume* volume)
{
	PackageFSRootWriteLocker writeLocker(this);

	volume->SetPackageFSRoot(this);

	fVolumes.Add(volume);
		// TODO: Correct order?

	if (fSystemVolume == NULL && volume->MountType() == MOUNT_TYPE_SYSTEM)
		fSystemVolume = volume;

	return B_OK;
}


void
PackageFSRoot::_RemoveVolume(Volume* volume)
{
	PackageFSRootWriteLocker writeLocker(this);

	if (volume == fSystemVolume)
		fSystemVolume = NULL;

	fVolumes.Remove(volume);

	volume->SetPackageFSRoot(NULL);
}


/*static*/ status_t
PackageFSRoot::_GetOrCreateRoot(dev_t deviceID, ino_t nodeID,
	PackageFSRoot*& _root)
{
	// first check the list, if the root already exists
	MutexLocker rootListLocker(sRootListLock);

	if (PackageFSRoot* root = _FindRootLocked(deviceID, nodeID)) {
		root->AcquireReference();
		_root = root;
		return B_OK;
	}

	rootListLocker.Unlock();

	// create a new root
	PackageFSRoot* root = new(std::nothrow) PackageFSRoot(deviceID, nodeID);
	if (root == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PackageFSRoot> rootDeleter(root);

	status_t error = root->Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the root -- first recheck whether someone else added the root in the
	// meantime
	rootListLocker.Lock();

	if (PackageFSRoot* otherRoot = _FindRootLocked(deviceID, nodeID)) {
		// indeed, someone was faster
		otherRoot->AcquireReference();
		_root = otherRoot;
		return B_OK;
	}

	sRootList.Add(root);

	_root = rootDeleter.Detach();
	return B_OK;
}


/*static*/ PackageFSRoot*
PackageFSRoot::_FindRootLocked(dev_t deviceID, ino_t nodeID)
{
	for (RootList::Iterator it = sRootList.GetIterator();
			PackageFSRoot* root = it.Next();) {
		if (root->DeviceID() == deviceID && root->NodeID() == nodeID)
			return root;
	}

	return NULL;
}


/*static*/ void
PackageFSRoot::_PutRoot(PackageFSRoot* root)
{
	// Only non-custom roots are in the global list.
	if (!root->IsCustom()) {
		MutexLocker rootListLocker(sRootListLock);

		// When releasing the last reference, remove the root from the list.
		if (root->CountReferences() == 1)
			sRootList.Remove(root);

		rootListLocker.Unlock();
	}

	root->ReleaseReference();
}
