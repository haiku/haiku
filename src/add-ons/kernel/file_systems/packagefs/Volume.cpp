/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include <new>

#include <AppDefs.h>
#include <driver_settings.h>
#include <KernelExport.h>
#include <NodeMonitor.h>
#include <package/PackageInfoAttributes.h>

#include <AutoDeleter.h>

#include <Notifications.h>
#include <vfs.h>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReaderImpl.h>

#include "DebugSupport.h"
#include "kernel_interface.h"
#include "PackageDirectory.h"
#include "PackageFile.h"
#include "PackageFSRoot.h"
#include "PackageLinkDirectory.h"
#include "PackageLinksDirectory.h"
#include "PackageSymlink.h"
#include "Resolvable.h"
#include "UnpackingLeafNode.h"
#include "UnpackingDirectory.h"
#include "Version.h"


using namespace BPackageKit;
using namespace BPackageKit::BHPKG;
using BPackageKit::BHPKG::BPrivate::PackageReaderImpl;


// node ID of the root directory
static const ino_t kRootDirectoryID = 1;

// shine-through directories
const char* const kSystemShineThroughDirectories[] = {
	"packages", NULL
};
const char* const kCommonShineThroughDirectories[] = {
	"non-packaged", "packages", "settings", "var", NULL
};
const char* const* kHomeShineThroughDirectories
	= kCommonShineThroughDirectories;


// #pragma mark - Job


struct Volume::Job : DoublyLinkedListLinkImpl<Job> {
	Job(Volume* volume)
		:
		fVolume(volume)
	{
	}

	virtual ~Job()
	{
	}

	virtual void Do() = 0;

protected:
	Volume*	fVolume;
};


// #pragma mark - AddPackageDomainJob


struct Volume::AddPackageDomainJob : Job {
	AddPackageDomainJob(Volume* volume, PackageDomain* domain)
		:
		Job(volume),
		fDomain(domain)
	{
		fDomain->AcquireReference();
	}

	virtual ~AddPackageDomainJob()
	{
		fDomain->ReleaseReference();
	}

	virtual void Do()
	{
		fVolume->_AddPackageDomain(fDomain, true);
	}

private:
	PackageDomain*	fDomain;
};


// #pragma mark - DomainDirectoryEventJob


struct Volume::DomainDirectoryEventJob : Job {
	DomainDirectoryEventJob(Volume* volume, PackageDomain* domain)
		:
		Job(volume),
		fDomain(domain),
		fEvent()
	{
		fDomain->AcquireReference();
	}

	virtual ~DomainDirectoryEventJob()
	{
		fDomain->ReleaseReference();
	}

	status_t Init(const KMessage* event)
	{
		RETURN_ERROR(fEvent.SetTo(event->Buffer(), -1,
			KMessage::KMESSAGE_CLONE_BUFFER));
	}

	virtual void Do()
	{
		fVolume->_DomainListenerEventOccurred(fDomain, &fEvent);
	}

private:
	PackageDomain*	fDomain;
	KMessage		fEvent;
};


// #pragma mark - PackageLoaderErrorOutput


struct Volume::PackageLoaderErrorOutput : BErrorOutput {
	PackageLoaderErrorOutput(Package* package)
		:
		fPackage(package)
	{
	}

	virtual void PrintErrorVarArgs(const char* format, va_list args)
	{
// TODO:...
	}

private:
	Package*	fPackage;
};


// #pragma mark - PackageLoaderContentHandler


struct Volume::PackageLoaderContentHandler : BPackageContentHandler {
	PackageLoaderContentHandler(Package* package)
		:
		fPackage(package),
		fErrorOccurred(false)
	{
	}

	status_t Init()
	{
		return B_OK;
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		if (fErrorOccurred)
			return B_OK;

		PackageDirectory* parentDir = NULL;
		if (entry->Parent() != NULL) {
			parentDir = dynamic_cast<PackageDirectory*>(
				(PackageNode*)entry->Parent()->UserToken());
			if (parentDir == NULL)
				RETURN_ERROR(B_BAD_DATA);
		}

		status_t error;

		// get the file mode -- filter out write permissions
		mode_t mode = entry->Mode() & ~(mode_t)(S_IWUSR | S_IWGRP | S_IWOTH);

		// create the package node
		PackageNode* node;
		if (S_ISREG(mode)) {
			// file
			node = new(std::nothrow) PackageFile(fPackage, mode, entry->Data());
		} else if (S_ISLNK(mode)) {
			// symlink
			PackageSymlink* symlink = new(std::nothrow) PackageSymlink(
				fPackage, mode);
			if (symlink == NULL)
				RETURN_ERROR(B_NO_MEMORY);

			error = symlink->SetSymlinkPath(entry->SymlinkPath());
			if (error != B_OK) {
				delete symlink;
				return error;
			}

			node = symlink;
		} else if (S_ISDIR(mode)) {
			// directory
			node = new(std::nothrow) PackageDirectory(fPackage, mode);
		} else
			RETURN_ERROR(B_BAD_DATA);

		if (node == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		BReference<PackageNode> nodeReference(node, true);

		error = node->Init(parentDir, entry->Name());
		if (error != B_OK)
			RETURN_ERROR(error);

		node->SetModifiedTime(entry->ModifiedTime());

		// add it to the parent directory
		if (parentDir != NULL)
			parentDir->AddChild(node);
		else
			fPackage->AddNode(node);

		entry->SetUserToken(node);

		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		if (fErrorOccurred)
			return B_OK;

		PackageNode* node = (PackageNode*)entry->UserToken();

		PackageNodeAttribute* nodeAttribute = new(std::nothrow)
			PackageNodeAttribute(node, attribute->Type(), attribute->Data());
		if (nodeAttribute == NULL)
			RETURN_ERROR(B_NO_MEMORY)

		status_t error = nodeAttribute->Init(attribute->Name());
		if (error != B_OK) {
			delete nodeAttribute;
			RETURN_ERROR(error);
		}

		node->AddAttribute(nodeAttribute);

		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		switch (value.attributeID) {
//            union {
//                 uint64          unsignedInt;
//                 const char*     string;
//                 BPackageVersionData version;
//                 BPackageResolvableData resolvable;
//                 BPackageResolvableExpressionData resolvableExpression;
//             };
			case B_PACKAGE_INFO_NAME:
				return fPackage->SetName(value.string);

			case B_PACKAGE_INFO_VERSION:
			{
				Version* version;
				status_t error = Version::Create(value.version.major,
					value.version.minor, value.version.micro,
					value.version.release, version);
				if (error != B_OK)
					RETURN_ERROR(error);

				fPackage->SetVersion(version);

				break;
			}

			case B_PACKAGE_INFO_PROVIDES:
			{
				// create a version object, if a version is specified
				Version* version = NULL;
				if (value.resolvable.haveVersion) {
					const BPackageVersionData& versionInfo
						= value.resolvable.version;
					status_t error = Version::Create(versionInfo.major,
						versionInfo.minor, versionInfo.micro,
						versionInfo.release, version);
					if (error != B_OK)
						RETURN_ERROR(error);
				}
				ObjectDeleter<Version> versionDeleter(version);

				// create the resolvable
				Resolvable* resolvable = new(std::nothrow) Resolvable(fPackage);
				if (resolvable == NULL)
					RETURN_ERROR(B_NO_MEMORY);
				ObjectDeleter<Resolvable> resolvableDeleter(resolvable);

				status_t error = resolvable->Init(value.resolvable.name,
					versionDeleter.Detach());
				if (error != B_OK)
					RETURN_ERROR(error);

				fPackage->AddResolvable(resolvableDeleter.Detach());

				break;
			}

			case B_PACKAGE_INFO_REQUIRES:
			{
				// create the dependency
				Dependency* dependency = new(std::nothrow) Dependency(fPackage);
				if (dependency == NULL)
					RETURN_ERROR(B_NO_MEMORY);
				ObjectDeleter<Dependency> dependencyDeleter(dependency);

				status_t error = dependency->Init(
					value.resolvableExpression.name);
				if (error != B_OK)
					RETURN_ERROR(error);

				// create a version object, if a version is specified
				Version* version = NULL;
				if (value.resolvableExpression.haveOpAndVersion) {
					const BPackageVersionData& versionInfo
						= value.resolvableExpression.version;
					status_t error = Version::Create(versionInfo.major,
						versionInfo.minor, versionInfo.micro,
						versionInfo.release, version);
					if (error != B_OK)
						RETURN_ERROR(error);

					dependency->SetVersionRequirement(
						value.resolvableExpression.op, version);
				}

				fPackage->AddDependency(dependencyDeleter.Detach());

				break;
			}

			default:
				break;
		}

		// TODO!
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
		fErrorOccurred = true;
	}

private:
	Package*	fPackage;
	bool		fErrorOccurred;
};


// #pragma mark - DomainDirectoryListener


struct Volume::DomainDirectoryListener : NotificationListener {
	DomainDirectoryListener(Volume* volume, PackageDomain* domain)
		:
		fVolume(volume),
		fDomain(domain)
	{
	}

	virtual void EventOccurred(NotificationService& service,
		const KMessage* event)
	{
		DomainDirectoryEventJob* job = new(std::nothrow)
			DomainDirectoryEventJob(fVolume, fDomain);
		if (job == NULL || job->Init(event)) {
			delete job;
			return;
		}

		fVolume->_PushJob(job);
	}

private:
	Volume*			fVolume;
	PackageDomain*	fDomain;
};


// #pragma mark - Volume


Volume::Volume(fs_volume* fsVolume)
	:
	fFSVolume(fsVolume),
	fRootDirectory(NULL),
	fPackageFSRoot(NULL),
	fPackageLoader(-1),
	fNextNodeID(kRootDirectoryID + 1),
	fTerminating(false)
{
	rw_lock_init(&fLock, "packagefs volume");
	mutex_init(&fJobQueueLock, "packagefs volume job queue");
	fJobQueueCondition.Init(this, "packagefs volume job queue");
}


Volume::~Volume()
{
	_TerminatePackageLoader();

	while (PackageDomain* packageDomain = fPackageDomains.Head())
		_RemovePackageDomain(packageDomain);

	// remove all nodes from the ID hash table
	Node* node = fNodes.Clear(true);
	while (node != NULL) {
		Node* next = node->IDHashTableNext();
		node->ReleaseReference();
		node = next;
	}

	if (fPackageFSRoot != NULL) {
		if (this == fPackageFSRoot->SystemVolume())
			_RemovePackageLinksDirectory();

		fPackageFSRoot->UnregisterVolume(this);
	}

	if (fRootDirectory != NULL)
		fRootDirectory->ReleaseReference();

	mutex_destroy(&fJobQueueLock);
	rw_lock_destroy(&fLock);
}


status_t
Volume::Mount(const char* parameterString)
{
	// init the node table
	status_t error = fNodes.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	const char* packages = NULL;
	const char* volumeName = NULL;
	const char* mountType = NULL;
	const char* shineThrough = NULL;

	void* parameterHandle = parse_driver_settings_string(parameterString);
	if (parameterHandle != NULL) {
		packages = get_driver_parameter(parameterHandle, "packages", NULL,
			NULL);
		volumeName = get_driver_parameter(parameterHandle, "volume-name", NULL,
			NULL);
		mountType = get_driver_parameter(parameterHandle, "type", NULL, NULL);
		shineThrough = get_driver_parameter(parameterHandle, "shine-through",
			NULL, NULL);
	}

	CObjectDeleter<void, status_t> parameterHandleDeleter(parameterHandle,
		&delete_driver_settings);

	if (packages == NULL || packages[0] == '\0') {
		ERROR("need package folder ('packages' parameter)!\n");
		RETURN_ERROR(B_BAD_VALUE);
	}

	error = _InitMountType(mountType);
	if (error != B_OK) {
		ERROR("invalid mount type: \"%s\"\n", mountType);
		RETURN_ERROR(B_ERROR);
	}

	struct stat st;
	if (stat(packages, &st) < 0)
		RETURN_ERROR(B_ERROR);

	// If no volume name is given, infer it from the mount type.
	if (volumeName == NULL) {
		switch (fMountType) {
			case MOUNT_TYPE_SYSTEM:
				volumeName = "system";
				break;
			case MOUNT_TYPE_COMMON:
				volumeName = "common";
				break;
			case MOUNT_TYPE_HOME:
				volumeName = "home";
				break;
			case MOUNT_TYPE_CUSTOM:
				volumeName = "Package FS";
				break;
		}
	}

	// create the root node
	fRootDirectory
		= new(std::nothrow) ::RootDirectory(kRootDirectoryID, st.st_mtim);
	if (fRootDirectory == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	fRootDirectory->Init(NULL, volumeName, 0);
	fNodes.Insert(fRootDirectory);

	// get our mount point
	error = vfs_get_mount_point(fFSVolume->id, &fMountPoint.deviceID,
		&fMountPoint.nodeID);
	if (error != B_OK)
		RETURN_ERROR(error);

	// register with packagefs root
	error = ::PackageFSRoot::RegisterVolume(this);
	if (error != B_OK)
		RETURN_ERROR(error);

	if (this == fPackageFSRoot->SystemVolume()) {
		error = _AddPackageLinksDirectory();
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	// create default package domain
	error = _AddInitialPackageDomain(packages);
	if (error != B_OK)
		RETURN_ERROR(error);

	// spawn package loader thread
	fPackageLoader = spawn_kernel_thread(&_PackageLoaderEntry,
		"package loader", B_NORMAL_PRIORITY, this);
	if (fPackageLoader < 0)
		RETURN_ERROR(fPackageLoader);

	// establish shine-through directories
	error = _CreateShineThroughDirectories(shineThrough);
	if (error != B_OK)
		RETURN_ERROR(error);

	// publish the root node
	fRootDirectory->AcquireReference();
	error = PublishVNode(fRootDirectory);
	if (error != B_OK) {
		fRootDirectory->ReleaseReference();
		RETURN_ERROR(error);
	}

	// run the package loader
	resume_thread(fPackageLoader);

	return B_OK;
}


void
Volume::Unmount()
{
	_TerminatePackageLoader();
}


status_t
Volume::GetVNode(ino_t nodeID, Node*& _node)
{
	return get_vnode(fFSVolume, nodeID, (void**)&_node);
}


status_t
Volume::PutVNode(ino_t nodeID)
{
	return put_vnode(fFSVolume, nodeID);
}


status_t
Volume::RemoveVNode(ino_t nodeID)
{
	return remove_vnode(fFSVolume, nodeID);
}


status_t
Volume::PublishVNode(Node* node)
{
	return publish_vnode(fFSVolume, node->ID(), node, &gPackageFSVnodeOps,
		node->Mode() & S_IFMT, 0);
}


status_t
Volume::AddPackageDomain(const char* path)
{
	PackageDomain* packageDomain = new(std::nothrow) PackageDomain(this);
	if (packageDomain == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	BReference<PackageDomain> packageDomainReference(packageDomain, true);

	status_t error = packageDomain->Init(path);
	if (error != B_OK)
		RETURN_ERROR(error);

	Job* job = new(std::nothrow) AddPackageDomainJob(this, packageDomain);
	if (job == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	_PushJob(job);

	return B_OK;
}


void
Volume::PackageLinkNodeAdded(Node* node)
{
	_AddPackageLinksNode(node);

	notify_entry_created(ID(), node->Parent()->ID(), node->Name(), node->ID());
}


void
Volume::PackageLinkNodeRemoved(Node* node)
{
	_RemovePackageLinksNode(node);

	notify_entry_removed(ID(), node->Parent()->ID(), node->Name(), node->ID());
}


void
Volume::PackageLinkNodeChanged(Node* node, uint32 statFields)
{
	notify_stat_changed(ID(), node->ID(), statFields);
}


/*static*/ status_t
Volume::_PackageLoaderEntry(void* data)
{
	return ((Volume*)data)->_PackageLoader();
}


status_t
Volume::_PackageLoader()
{
	while (!fTerminating) {
		MutexLocker jobQueueLocker(fJobQueueLock);

		Job* job = fJobQueue.RemoveHead();
		if (job == NULL) {
			// no job yet -- wait for someone notifying us
			ConditionVariableEntry waitEntry;
			fJobQueueCondition.Add(&waitEntry);
			jobQueueLocker.Unlock();
			waitEntry.Wait();
			continue;
		}

		// do the job
		jobQueueLocker.Unlock();
		job->Do();
		delete job;
	}

	return B_OK;
}


void
Volume::_TerminatePackageLoader()
{
	fTerminating = true;

	if (fPackageLoader >= 0) {
		MutexLocker jobQueueLocker(fJobQueueLock);
		fJobQueueCondition.NotifyOne();
		jobQueueLocker.Unlock();

		wait_for_thread(fPackageLoader, NULL);
		fPackageLoader = -1;
	}

	// empty the job queue
	while (Job* job = fJobQueue.RemoveHead())
		delete job;
}


void
Volume::_PushJob(Job* job)
{
	MutexLocker jobQueueLocker(fJobQueueLock);
	fJobQueue.Add(job);
	fJobQueueCondition.NotifyOne();
}


status_t
Volume::_AddInitialPackageDomain(const char* path)
{
	PackageDomain* domain = new(std::nothrow) PackageDomain(this);
	if (domain == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	BReference<PackageDomain> domainReference(domain, true);

	status_t error = domain->Init(path);
	if (error != B_OK)
		RETURN_ERROR(error);

	return _AddPackageDomain(domain, false);
}


status_t
Volume::_AddPackageDomain(PackageDomain* domain, bool notify)
{
	// create a directory listener
	DomainDirectoryListener* listener = new(std::nothrow)
		DomainDirectoryListener(this, domain);
	if (listener == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	// prepare the package domain
	status_t error = domain->Prepare(listener);
	if (error != B_OK) {
		ERROR("Failed to prepare package domain \"%s\": %s\n",
			domain->Path(), strerror(errno));
		RETURN_ERROR(errno);
	}

	// iterate through the dir and create packages
	DIR* dir = opendir(domain->Path());
	if (dir == NULL) {
		ERROR("Failed to open package domain directory \"%s\": %s\n",
			domain->Path(), strerror(errno));
		RETURN_ERROR(errno);
	}
	CObjectDeleter<DIR, int> dirCloser(dir, closedir);

	while (dirent* entry = readdir(dir)) {
		// skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		_DomainEntryCreated(domain, domain->DeviceID(), domain->NodeID(),
			-1, entry->d_name, false, notify);
// TODO: -1 node ID?
	}

	// add the packages to the node tree
	VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
	VolumeWriteLocker volumeLocker(this);
	for (PackageFileNameHashTable::Iterator it
			= domain->Packages().GetIterator(); Package* package = it.Next();) {
		error = _AddPackageContent(package, notify);
		if (error != B_OK) {
			for (it.Rewind(); Package* activePackage = it.Next();) {
				if (activePackage == package)
					break;
				_RemovePackageContent(activePackage, NULL, notify);
			}
			RETURN_ERROR(error);
		}
	}

	fPackageDomains.Add(domain);
	domain->AcquireReference();

	return B_OK;
}


void
Volume::_RemovePackageDomain(PackageDomain* domain)
{
	// remove the domain's packages from the node tree
	VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
	VolumeWriteLocker volumeLocker(this);
	for (PackageFileNameHashTable::Iterator it
			= domain->Packages().GetIterator(); Package* package = it.Next();) {
		_RemovePackageContent(package, NULL, false);
	}

	// remove the domain
	fPackageDomains.Remove(domain);
	domain->ReleaseReference();
}


status_t
Volume::_LoadPackage(Package* package)
{
	// open package file
	int fd = package->Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(package);

	// initialize package reader
	PackageLoaderErrorOutput errorOutput(package);
	PackageReaderImpl packageReader(&errorOutput);
	status_t error = packageReader.Init(fd, false);
	if (error != B_OK)
		RETURN_ERROR(error);

	// parse content
	PackageLoaderContentHandler handler(package);
	error = handler.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = packageReader.ParseContent(&handler);
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}


status_t
Volume::_AddPackageContent(Package* package, bool notify)
{
	status_t error = fPackageFSRoot->AddPackage(package);
	if (error != B_OK)
		RETURN_ERROR(error);

	for (PackageNodeList::Iterator it = package->Nodes().GetIterator();
			PackageNode* node = it.Next();) {
		// skip over ".PackageInfo" file, it isn't part of the package content
		if (strcmp(node->Name(), B_HPKG_PACKAGE_INFO_FILE_NAME) == 0)
			continue;
		error = _AddPackageContentRootNode(package, node, notify);
		if (error != B_OK) {
			_RemovePackageContent(package, node, notify);
			RETURN_ERROR(error);
		}
	}

	return B_OK;
}


void
Volume::_RemovePackageContent(Package* package, PackageNode* endNode,
	bool notify)
{
	PackageNode* node = package->Nodes().Head();
	while (node != NULL) {
		if (node == endNode)
			break;

		PackageNode* nextNode = package->Nodes().GetNext(node);

		// skip over ".PackageInfo" file, it isn't part of the package content
		if (strcmp(node->Name(), B_HPKG_PACKAGE_INFO_FILE_NAME) != 0)
			_RemovePackageContentRootNode(package, node, NULL, notify);

		node = nextNode;
	}

	fPackageFSRoot->RemovePackage(package);;
}


/*!	This method recursively iterates through the descendents of the given
	package root node and adds all package nodes to the node tree in
	pre-order.
	Due to limited kernel stack space we avoid deep recursive function calls
	and rather use the package node stack implied by the tree.
*/
status_t
Volume::_AddPackageContentRootNode(Package* package,
	PackageNode* rootPackageNode, bool notify)
{
	PackageNode* packageNode = rootPackageNode;
	Directory* directory = fRootDirectory;
	directory->WriteLock();

	do {
		Node* node;
		status_t error = _AddPackageNode(directory, packageNode, notify, node);
		if (error != B_OK) {
			// unlock all directories
			while (directory != NULL) {
				directory->WriteUnlock();
				directory = directory->Parent();
			}

			// remove the added package nodes
			_RemovePackageContentRootNode(package, rootPackageNode, packageNode,
				notify);
			RETURN_ERROR(error);
		}

		// recursive into directory
		if (PackageDirectory* packageDirectory
				= dynamic_cast<PackageDirectory*>(packageNode)) {
			if (packageDirectory->FirstChild() != NULL) {
				directory = dynamic_cast<Directory*>(node);
				packageNode = packageDirectory->FirstChild();
				directory->WriteLock();
				continue;
			}
		}

		// continue with the next available (ancestors's) sibling
		do {
			PackageDirectory* packageDirectory = packageNode->Parent();
			PackageNode* sibling = packageDirectory != NULL
				? packageDirectory->NextChild(packageNode) : NULL;

			if (sibling != NULL) {
				packageNode = sibling;
				break;
			}

			// no more siblings -- go back up the tree
			packageNode = packageDirectory;
			directory->WriteUnlock();
			directory = directory->Parent();
				// the parent is still locked, so this is safe
		} while (packageNode != NULL);
	} while (packageNode != NULL);

	return B_OK;
}


/*!	Recursively iterates through the descendents of the given package root node
	and removes all package nodes from the node tree in post-order, until
	encountering \a endPackageNode (if non-null).
	Due to limited kernel stack space we avoid deep recursive function calls
	and rather use the package node stack implied by the tree.
*/
void
Volume::_RemovePackageContentRootNode(Package* package,
	PackageNode* rootPackageNode, PackageNode* endPackageNode, bool notify)
{
	PackageNode* packageNode = rootPackageNode;
	Directory* directory = fRootDirectory;
	directory->WriteLock();

	do {
		if (packageNode == endPackageNode)
			break;

		// recursive into directory
		if (PackageDirectory* packageDirectory
				= dynamic_cast<PackageDirectory*>(packageNode)) {
			if (packageDirectory->FirstChild() != NULL) {
				directory = dynamic_cast<Directory*>(
					directory->FindChild(packageNode->Name()));
				packageNode = packageDirectory->FirstChild();
				directory->WriteLock();
				continue;
			}
		}

		// continue with the next available (ancestors's) sibling
		do {
			PackageDirectory* packageDirectory = packageNode->Parent();
			PackageNode* sibling = packageDirectory != NULL
				? packageDirectory->NextChild(packageNode) : NULL;

			// we're done with the node -- remove it
			_RemovePackageNode(directory, packageNode,
				directory->FindChild(packageNode->Name()), notify);

			if (sibling != NULL) {
				packageNode = sibling;
				break;
			}

			// no more siblings -- go back up the tree
			packageNode = packageDirectory;
			directory->WriteUnlock();
			directory = directory->Parent();
				// the parent is still locked, so this is safe
		} while (packageNode != NULL/* && packageNode != rootPackageNode*/);
	} while (packageNode != NULL/* && packageNode != rootPackageNode*/);
}


status_t
Volume::_AddPackageNode(Directory* directory, PackageNode* packageNode,
	bool notify, Node*& _node)
{
	bool newNode = false;
	UnpackingNode* unpackingNode;
	Node* node = directory->FindChild(packageNode->Name());

	if (node != NULL) {
		unpackingNode = dynamic_cast<UnpackingNode*>(node);
		if (unpackingNode == NULL)
			RETURN_ERROR(B_BAD_VALUE);
	} else {
		status_t error = _CreateUnpackingNode(packageNode->Mode(), directory,
			packageNode->Name(), unpackingNode);
		if (error != B_OK)
			RETURN_ERROR(error);

		node = unpackingNode->GetNode();
		newNode = true;
	}

	BReference<Node> nodeReference(node);
	NodeWriteLocker nodeWriteLocker(node);

	status_t error = unpackingNode->AddPackageNode(packageNode);
	if (error != B_OK) {
		// remove the node, if created before
		if (newNode)
			_RemoveNode(node);
		RETURN_ERROR(error);
	}

	if (notify) {
		if (newNode) {
			notify_entry_created(ID(), directory->ID(), node->Name(),
				node->ID());
		} else if (packageNode == unpackingNode->GetPackageNode()) {
			// The new package node has become the one representing the node.
			// Send stat changed notification for directories and entry
			// removed + created notifications for files and symlinks.
			if (S_ISDIR(packageNode->Mode())) {
				notify_stat_changed(ID(), node->ID(),
					B_STAT_MODE | B_STAT_UID | B_STAT_GID | B_STAT_SIZE
						| B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME
						| B_STAT_CREATION_TIME | B_STAT_CHANGE_TIME);
				// TODO: Actually the attributes might change, too!
			} else {
				notify_entry_removed(ID(), directory->ID(), node->Name(),
					node->ID());
				notify_entry_created(ID(), directory->ID(), node->Name(),
					node->ID());
			}
		}
	}

	_node = node;
	return B_OK;
}


void
Volume::_RemovePackageNode(Directory* directory, PackageNode* packageNode,
	Node* node, bool notify)
{
	UnpackingNode* unpackingNode = dynamic_cast<UnpackingNode*>(node);
	if (unpackingNode == NULL)
		return;

	BReference<Node> nodeReference(node);
	NodeWriteLocker nodeWriteLocker(node);

	PackageNode* headPackageNode = unpackingNode->GetPackageNode();
	unpackingNode->RemovePackageNode(packageNode);

	// If the node doesn't have any more package nodes attached, remove it
	// completely.
	bool nodeRemoved = false;
	if (unpackingNode->GetPackageNode() == NULL) {
		// we get and put the vnode to notify the VFS
		// TODO: We should probably only do that, if the node is known to the
		// VFS in the first place.
		Node* dummyNode;
		bool gotVNode = GetVNode(node->ID(), dummyNode) == B_OK;

		_RemoveNode(node);
		nodeRemoved = true;

		if (gotVNode) {
			RemoveVNode(node->ID());
			PutVNode(node->ID());
		}
	}

	if (!notify)
		return;

	// send notifications
	if (nodeRemoved) {
		notify_entry_removed(ID(), directory->ID(), node->Name(), node->ID());
	} else if (packageNode == headPackageNode) {
		// The new package node was the one representing the node.
		// Send stat changed notification for directories and entry
		// removed + created notifications for files and symlinks.
		if (S_ISDIR(packageNode->Mode())) {
			notify_stat_changed(ID(), node->ID(),
				B_STAT_MODE | B_STAT_UID | B_STAT_GID | B_STAT_SIZE
					| B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME
					| B_STAT_CREATION_TIME | B_STAT_CHANGE_TIME);
			// TODO: Actually the attributes might change, too!
		} else {
			notify_entry_removed(ID(), directory->ID(), node->Name(),
				node->ID());
			notify_entry_created(ID(), directory->ID(), node->Name(),
				node->ID());
		}
	}
}


status_t
Volume::_CreateUnpackingNode(mode_t mode, Directory* parent, const char* name,
	UnpackingNode*& _node)
{
	UnpackingNode* unpackingNode;
	if (S_ISREG(mode) || S_ISLNK(mode))
		unpackingNode = new(std::nothrow) UnpackingLeafNode(fNextNodeID++);
	else if (S_ISDIR(mode))
		unpackingNode = new(std::nothrow) UnpackingDirectory(fNextNodeID++);
	else
		RETURN_ERROR(B_UNSUPPORTED);

	if (unpackingNode == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	Node* node = unpackingNode->GetNode();
	BReference<Node> nodeReference(node, true);

	status_t error = node->Init(parent, name, 0);
	if (error != B_OK)
		RETURN_ERROR(error);

	parent->AddChild(node);

	fNodes.Insert(node);
	nodeReference.Detach();
		// we keep the initial node reference for this table

	_node = unpackingNode;
	return B_OK;
}


void
Volume::_RemoveNode(Node* node)
{
	// remove from parent
	Directory* parent = node->Parent();
	parent->RemoveChild(node);

	// remove from node table
	fNodes.Remove(node);
	node->ReleaseReference();
}


void
Volume::_DomainListenerEventOccurred(PackageDomain* domain,
	const KMessage* event)
{
	int32 opcode;
	if (event->What() != B_NODE_MONITOR
		|| event->FindInt32("opcode", &opcode) != B_OK) {
		return;
	}

	switch (opcode) {
		case B_ENTRY_CREATED:
		{
			int32 device;
			int64 directory;
			int64 node;
			const char* name;
			if (event->FindInt32("device", &device) == B_OK
				&& event->FindInt64("directory", &directory) == B_OK
				&& event->FindInt64("node", &node) == B_OK
				&& event->FindString("name", &name) == B_OK) {
				_DomainEntryCreated(domain, device, directory, node, name,
					true, true);
			}
			break;
		}

		case B_ENTRY_REMOVED:
		{
			int32 device;
			int64 directory;
			int64 node;
			const char* name;
			if (event->FindInt32("device", &device) == B_OK
				&& event->FindInt64("directory", &directory) == B_OK
				&& event->FindInt64("node", &node) == B_OK
				&& event->FindString("name", &name) == B_OK) {
				_DomainEntryRemoved(domain, device, directory, node, name,
					true);
			}
			break;
		}

		case B_ENTRY_MOVED:
		{
			int32 device;
			int64 fromDirectory;
			int64 toDirectory;
			int32 nodeDevice;
			int64 node;
			const char* fromName;
			const char* name;
			if (event->FindInt32("device", &device) == B_OK
				&& event->FindInt64("from directory", &fromDirectory) == B_OK
				&& event->FindInt64("to directory", &toDirectory) == B_OK
				&& event->FindInt32("node device", &nodeDevice) == B_OK
				&& event->FindInt64("node", &node) == B_OK
				&& event->FindString("from name", &fromName) == B_OK
				&& event->FindString("name", &name) == B_OK) {
				_DomainEntryMoved(domain, device, fromDirectory, toDirectory,
					nodeDevice, node, fromName, name, true);
			}
			break;
		}

		default:
			break;
	}
}


void
Volume::_DomainEntryCreated(PackageDomain* domain, dev_t deviceID,
	ino_t directoryID, ino_t nodeID, const char* name, bool addContent,
	bool notify)
{
	// let's see, if things look plausible
	if (deviceID != domain->DeviceID() || directoryID != domain->NodeID()
		|| domain->FindPackage(name) != NULL) {
		return;
	}

	// check whether the entry is a file
	struct stat st;
	if (fstatat(domain->DirectoryFD(), name, &st, AT_SYMLINK_NOFOLLOW) < 0
		|| !S_ISREG(st.st_mode)) {
		return;
	}

	// create a package
	Package* package = new(std::nothrow) Package(domain, st.st_dev, st.st_ino);
	if (package == NULL)
		return;
	BReference<Package> packageReference(package, true);

	status_t error = package->Init(name);
	if (error != B_OK)
		return;

	error = _LoadPackage(package);
	if (error != B_OK)
		return;

	VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
	VolumeWriteLocker volumeLocker(this);
	domain->AddPackage(package);

	// add the package to the node tree
	if (addContent) {
		error = _AddPackageContent(package, notify);
		if (error != B_OK) {
			domain->RemovePackage(package);
			return;
		}
	}
}


void
Volume::_DomainEntryRemoved(PackageDomain* domain, dev_t deviceID,
	ino_t directoryID, ino_t nodeID, const char* name, bool notify)
{
	// let's see, if things look plausible
	if (deviceID != domain->DeviceID() || directoryID != domain->NodeID())
		return;

	Package* package = domain->FindPackage(name);
	if (package == NULL)
		return;
	BReference<Package> packageReference(package);

	// remove the package
	VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
	VolumeWriteLocker volumeLocker(this);
	_RemovePackageContent(package, NULL, true);
	domain->RemovePackage(package);
}


void
Volume::_DomainEntryMoved(PackageDomain* domain, dev_t deviceID,
	ino_t fromDirectoryID, ino_t toDirectoryID, dev_t nodeDeviceID,
	ino_t nodeID, const char* fromName, const char* name, bool notify)
{
	_DomainEntryRemoved(domain, deviceID, fromDirectoryID, nodeID, fromName,
		notify);
	_DomainEntryCreated(domain, deviceID, toDirectoryID, nodeID, name, true,
		notify);
}


status_t
Volume::_InitMountType(const char* mountType)
{
	if (mountType == NULL)
		fMountType = MOUNT_TYPE_CUSTOM;
	else if (strcmp(mountType, "system") == 0)
		fMountType = MOUNT_TYPE_SYSTEM;
	else if (strcmp(mountType, "common") == 0)
		fMountType = MOUNT_TYPE_COMMON;
	else if (strcmp(mountType, "home") == 0)
		fMountType = MOUNT_TYPE_HOME;
	else if (strcmp(mountType, "custom") == 0)
		fMountType = MOUNT_TYPE_CUSTOM;
	else
		RETURN_ERROR(B_BAD_VALUE);

	return B_OK;
}


status_t
Volume::_CreateShineThroughDirectories(const char* shineThroughSetting)
{
	// get the directories to map
	const char* const* directories = NULL;

	if (shineThroughSetting == NULL) {
		// nothing specified -- derive from mount type
		switch (fMountType) {
			case MOUNT_TYPE_SYSTEM:
				directories = kSystemShineThroughDirectories;
				break;
			case MOUNT_TYPE_COMMON:
				directories = kCommonShineThroughDirectories;
				break;
			case MOUNT_TYPE_HOME:
				directories = kHomeShineThroughDirectories;
				break;
			case MOUNT_TYPE_CUSTOM:
				return B_OK;
		}
	} else if (strcmp(shineThroughSetting, "system") == 0)
		directories = kSystemShineThroughDirectories;
	else if (strcmp(shineThroughSetting, "common") == 0)
		directories = kCommonShineThroughDirectories;
	else if (strcmp(shineThroughSetting, "home") == 0)
		directories = kHomeShineThroughDirectories;
	else if (strcmp(shineThroughSetting, "none") == 0)
		directories = NULL;
	else
		RETURN_ERROR(B_BAD_VALUE);

	if (directories == NULL)
		return B_OK;

	// iterate through the directory list, create the directories, and bind them
	// to the mount point subdirectories
	while (const char* directoryName = *(directories++)) {
		// look up the mount point subdirectory
		struct vnode* vnode;
		status_t error = vfs_entry_ref_to_vnode(fMountPoint.deviceID,
			fMountPoint.nodeID, directoryName, &vnode);
		if (error != B_OK) {
			dprintf("packagefs: Failed to get shine-through directory \"%s\": "
				"%s\n", directoryName, strerror(error));
			continue;
		}
		CObjectDeleter<struct vnode> vnodePutter(vnode, &vfs_put_vnode);

		// stat it
		struct stat st;
		error = vfs_stat_vnode(vnode, &st);
		if (error != B_OK) {
			dprintf("packagefs: Failed to stat shine-through directory \"%s\": "
				"%s\n", directoryName, strerror(error));
			continue;
		}

		if (!S_ISDIR(st.st_mode)) {
			dprintf("packagefs: Shine-through entry \"%s\" is not a "
				"directory\n", directoryName);
			continue;
		}

		// create the directory
		UnpackingNode* directory;
		error = _CreateUnpackingNode(S_IFDIR, fRootDirectory, directoryName,
			directory);
		if (error != B_OK)
			RETURN_ERROR(error);

		// publish its vnode, so the VFS will find it without asking us
		Node* directoryNode = directory->GetNode();
		error = PublishVNode(directoryNode);
		if (error != B_OK) {
			_RemoveNode(directoryNode);
			RETURN_ERROR(error);
		}

		// bind the directory
		error = vfs_bind_mount_directory(st.st_dev, st.st_ino, fFSVolume->id,
			directoryNode->ID());

		PutVNode(directoryNode->ID());
			// release our reference again -- on success
			// vfs_bind_mount_directory() got one

		if (error != B_OK)
			RETURN_ERROR(error);
	}

	return B_OK;
}


status_t
Volume::_AddPackageLinksDirectory()
{
	// called when mounting, so we don't need to lock the volume

	PackageLinksDirectory* packageLinksDirectory
		= fPackageFSRoot->GetPackageLinksDirectory();

	NodeWriteLocker rootDirectoryWriteLocker(fRootDirectory);
	NodeWriteLocker packageLinksDirectoryWriteLocker(packageLinksDirectory);

	packageLinksDirectory->SetParent(fRootDirectory);
	fRootDirectory->AddChild(packageLinksDirectory);

	_AddPackageLinksNode(packageLinksDirectory);

	packageLinksDirectory->SetListener(this);

	return B_OK;
}


void
Volume::_RemovePackageLinksDirectory()
{
	PackageLinksDirectory* packageLinksDirectory
		= fPackageFSRoot->GetPackageLinksDirectory();

	VolumeWriteLocker volumeLocker(this);
	NodeWriteLocker rootDirectoryWriteLocker(fRootDirectory);
	NodeWriteLocker packageLinksDirectoryWriteLocker(packageLinksDirectory);

	if (packageLinksDirectory->Parent() == fRootDirectory) {
		packageLinksDirectory->SetListener(NULL);
		_RemovePackageLinksNode(packageLinksDirectory);
		packageLinksDirectory->SetParent(NULL);
	}
}


void
Volume::_AddPackageLinksNode(Node* node)
{
	node->SetID(fNextNodeID++);

	fNodes.Insert(node);
	node->AcquireReference();

	// If this is a directory, recursively add descendants. The directory tree
	// for the package links isn't deep, so we can do recursion.
	if (Directory* directory = dynamic_cast<Directory*>(node)) {
		for (Node* child = directory->FirstChild(); child != NULL;
				child = directory->NextChild(child)) {
			NodeWriteLocker childWriteLocker(child);
			_AddPackageLinksNode(child);
		}
	}
}


void
Volume::_RemovePackageLinksNode(Node* node)
{
	// If this is a directory, recursively remove descendants. The directory
	// tree for the package links isn't deep, so we can do recursion.
	if (Directory* directory = dynamic_cast<Directory*>(node)) {
		for (Node* child = directory->FirstChild(); child != NULL;
				child = directory->NextChild(child)) {
			NodeWriteLocker childWriteLocker(child);
			_RemovePackageLinksNode(child);
		}
	}

	fNodes.Remove(node);
	node->ReleaseReference();
}


inline Volume*
Volume::_SystemVolumeIfNotSelf() const
{
	if (Volume* systemVolume = fPackageFSRoot->SystemVolume())
		return systemVolume == this ? NULL : systemVolume;
	return NULL;
}
