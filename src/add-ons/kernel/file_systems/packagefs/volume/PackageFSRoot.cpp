/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageFSRoot.h"

#include <AutoDeleterDrivers.h>

#include <vfs.h>

#include "DebugSupport.h"
#include "PackageLinksDirectory.h"
#include "StringConstants.h"


//#define TRACE_DEPENDENCIES_ENABLED
#ifdef TRACE_DEPENDENCIES_ENABLED
#	define TRACE_DEPENDENCIES(x...)	TPRINT(x)
#else
#	define TRACE_DEPENDENCIES(x...)	do {} while (false)
#endif


mutex PackageFSRoot::sRootListLock = MUTEX_INITIALIZER("packagefs root list");
PackageFSRoot::RootList PackageFSRoot::sRootList;


PackageFSRoot::PackageFSRoot(dev_t deviceID, ino_t nodeID)
	:
	fDeviceID(deviceID),
	fNodeID(nodeID),
	fSystemVolume(NULL),
	fPackageLinksDirectory(NULL)
{
	rw_lock_init(&fLock, "packagefs root");
}


PackageFSRoot::~PackageFSRoot()
{
	if (fPackageLinksDirectory != NULL)
		fPackageLinksDirectory->ReleaseReference();

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
	// create package links directory
	fPackageLinksDirectory = new(std::nothrow) PackageLinksDirectory;
	if (fPackageLinksDirectory == NULL)
		return B_NO_MEMORY;

	status_t error = fPackageLinksDirectory->Init(NULL,
		StringConstants::Get().kPackageLinksDirectoryName);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = fResolvables.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = fDependencies.Init();
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
		case PACKAGE_FS_MOUNT_TYPE_SYSTEM:
			relativeRootPath = "..";
			break;
		case PACKAGE_FS_MOUNT_TYPE_HOME:
			relativeRootPath = "../..";
			break;
		case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
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
		VnodePutter vnodePutter(vnode);

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
	PackageFSRootWriteLocker writeLocker(this);

	status_t error = _AddPackage(package);
	if (error != B_OK) {
		_RemovePackage(package);
		RETURN_ERROR(error);
	}

	return B_OK;
}


void
PackageFSRoot::RemovePackage(Package* package)
{
	PackageFSRootWriteLocker writeLocker(this);

	_RemovePackage(package);
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

	if (fSystemVolume == NULL && volume->MountType()
			== PACKAGE_FS_MOUNT_TYPE_SYSTEM) {
		fSystemVolume = volume;
	}

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


status_t
PackageFSRoot::_AddPackage(Package* package)
{
	TRACE_DEPENDENCIES("adding package \"%s\"\n", package->Name().Data());

	ResolvableDependencyList dependenciesToUpdate;

	// register resolvables
	for (ResolvableList::ConstIterator it
				= package->Resolvables().GetIterator();
			Resolvable* resolvable = it.Next();) {
		TRACE_DEPENDENCIES("  adding resolvable \"%s\"\n",
			resolvable->Name().Data());

		if (ResolvableFamily* family
				= fResolvables.Lookup(resolvable->Name())) {
			family->AddResolvable(resolvable, dependenciesToUpdate);
		} else {
			family = new(std::nothrow) ResolvableFamily;
			if (family == NULL)
				return B_NO_MEMORY;

			family->AddResolvable(resolvable, dependenciesToUpdate);
			fResolvables.Insert(family);

			// add pre-existing dependencies for that resolvable
			if (DependencyFamily* dependencyFamily
					= fDependencies.Lookup(resolvable->Name())) {
				dependencyFamily->AddDependenciesToList(dependenciesToUpdate);
			}
		}
	}

	// register dependencies
	for (DependencyList::ConstIterator it
				= package->Dependencies().GetIterator();
			Dependency* dependency = it.Next();) {
		TRACE_DEPENDENCIES("  adding dependency \"%s\"\n",
			dependency->Name().Data());

		if (DependencyFamily* family
				= fDependencies.Lookup(dependency->Name())) {
			family->AddDependency(dependency);
		} else {
			family = new(std::nothrow) DependencyFamily;
			if (family == NULL)
				return B_NO_MEMORY;

			family->AddDependency(dependency);
			fDependencies.Insert(family);
		}

		dependenciesToUpdate.Add(dependency);
	}

	status_t error = fPackageLinksDirectory->AddPackage(package);
	if (error != B_OK)
		RETURN_ERROR(error);

	_ResolveDependencies(dependenciesToUpdate);

	return B_OK;
}


void
PackageFSRoot::_RemovePackage(Package* package)
{
	TRACE_DEPENDENCIES("removing package \"%s\"\n", package->Name().Data());

	fPackageLinksDirectory->RemovePackage(package);

	// unregister dependencies
	for (DependencyList::ConstIterator it
				= package->Dependencies().GetIterator();
			Dependency* dependency = it.Next();) {
		if (DependencyFamily* family = dependency->Family()) {
			TRACE_DEPENDENCIES("  removing dependency \"%s\"\n",
				dependency->Name().Data());

			if (family->IsLastDependency(dependency)) {
				fDependencies.Remove(family);
				family->RemoveDependency(dependency);
				delete family;
			} else
				family->RemoveDependency(dependency);
		}

		if (Resolvable* resolvable = dependency->Resolvable())
			resolvable->RemoveDependency(dependency);
	}

	// unregister resolvables
	ResolvableDependencyList dependenciesToUpdate;

	for (ResolvableList::ConstIterator it
				= package->Resolvables().GetIterator();
			Resolvable* resolvable = it.Next();) {
		if (ResolvableFamily* family = resolvable->Family()) {
			TRACE_DEPENDENCIES("  removing resolvable \"%s\"\n",
				resolvable->Name().Data());

			if (family->IsLastResolvable(resolvable)) {
				fResolvables.Remove(family);
				family->RemoveResolvable(resolvable, dependenciesToUpdate);
				delete family;
			} else
				family->RemoveResolvable(resolvable, dependenciesToUpdate);
		}
	}

	_ResolveDependencies(dependenciesToUpdate);
}


void
PackageFSRoot::_ResolveDependencies(ResolvableDependencyList& dependencies)
{
	if (dependencies.IsEmpty())
		return;

	while (Dependency* dependency = dependencies.RemoveHead()) {
		Package* package = dependency->Package();
		_ResolveDependency(dependency);

		// also resolve all other dependencies for that package
		for (ResolvableDependencyList::Iterator it = dependencies.GetIterator();
				(dependency = it.Next()) != NULL;) {
			if (dependency->Package() == package) {
				it.Remove();
				_ResolveDependency(dependency);
			}
		}

		fPackageLinksDirectory->UpdatePackageDependencies(package);
	}
}


void
PackageFSRoot::_ResolveDependency(Dependency* dependency)
{
	TRACE_DEPENDENCIES("  resolving dependency \"%s\" (package \"%s\")\n",
		dependency->Name().Data(), dependency->Package()->Name().Data());

	// get the resolvable family for the dependency
	ResolvableFamily* resolvableFamily
		= fResolvables.Lookup(dependency->Name());
	if (resolvableFamily == NULL) {
		TRACE_DEPENDENCIES("    -> dependency \"%s\" unresolved\n",
			dependency->Name().Data());
		return;
	}

	// let the family resolve the dependency
	if (!resolvableFamily->ResolveDependency(dependency)) {
		TRACE_DEPENDENCIES("    -> dependency \"%s\" unresolved (version "
			"mismatch)\n", dependency->Name().Data());
	}
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
