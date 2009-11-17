/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include <new>

#include <KernelExport.h>

#include <AutoDeleter.h>

#include "ErrorOutput.h"
#include "FDCloser.h"
#include "PackageEntry.h"
#include "PackageReader.h"

#include "DebugSupport.h"
#include "Directory.h"
#include "LeafNode.h"
#include "kernel_interface.h"
#include "PackageDirectory.h"
#include "PackageFile.h"
#include "PackageSymlink.h"


// node ID of the root directory
static const ino_t kRootDirectoryID = 1;


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
	}

	virtual ~AddPackageDomainJob()
	{
	}

	virtual void Do()
	{
		fVolume->_AddPackageDomain(fDomain);
		fDomain = NULL;
	}

private:
	PackageDomain*	fDomain;
};


// #pragma mark - PackageLoaderErrorOutput


struct Volume::PackageLoaderErrorOutput : ErrorOutput {
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


struct Volume::PackageLoaderContentHandler : PackageContentHandler {
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

	virtual status_t HandleEntry(PackageEntry* entry)
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

		// create the package node
		PackageNode* node;
		if (S_ISREG(entry->Mode())) {
			// file
			node = new(std::nothrow) PackageFile(entry->Mode(), entry->Data());
		} else if (S_ISLNK(entry->Mode())) {
			// symlink
			PackageSymlink* symlink = new(std::nothrow) PackageSymlink(
				entry->Mode());
			if (symlink == NULL)
				RETURN_ERROR(B_NO_MEMORY);

			error = symlink->SetSymlinkPath(entry->SymlinkPath());
			if (error != B_OK) {
				delete symlink;
				return error;
			}

			node = symlink;
		} else if (S_ISDIR(entry->Mode())) {
			// directory
			node = new(std::nothrow) PackageDirectory(entry->Mode());
		} else
			RETURN_ERROR(B_BAD_DATA);

		if (node == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		ObjectDeleter<PackageNode> nodeDeleter(node);

		error = node->Init(parentDir, entry->Name());
		if (error != B_OK)
			RETURN_ERROR(error);

		// add it to the parent directory
		if (parentDir != NULL)
			parentDir->AddChild(node);
		else
			fPackage->AddNode(node);

		nodeDeleter.Detach();
		entry->SetUserToken(node);

		return B_OK;
	}

	virtual status_t HandleEntryAttribute(PackageEntry* entry,
		PackageEntryAttribute* attribute)
	{
		// TODO:...
		return B_OK;
	}

	virtual status_t HandleEntryDone(PackageEntry* entry)
	{
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


// #pragma mark - Volume


Volume::Volume(fs_volume* fsVolume)
	:
	fFSVolume(fsVolume),
	fRootDirectory(NULL),
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

	while (PackageDomain* packageDomain = fPackageDomains.RemoveHead())
		delete packageDomain;

	if (fRootDirectory != NULL)
		fRootDirectory->ReleaseReference();

	mutex_destroy(&fJobQueueLock);
	rw_lock_destroy(&fLock);
}


status_t
Volume::Mount()
{
	// init the node table
	status_t error = fNodes.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the root node
	fRootDirectory = new(std::nothrow) Directory(kRootDirectoryID);
	if (fRootDirectory == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	fNodes.Insert(fRootDirectory);

	// create default package domains
// TODO: Get them from the mount parameters instead!
	error = _AddInitialPackageDomain("/boot/common/packages");
	if (error != B_OK)
		RETURN_ERROR(error);

	// spawn package loader thread
	fPackageLoader = spawn_kernel_thread(&_PackageLoaderEntry,
		"package loader", B_NORMAL_PRIORITY, this);
	if (fPackageLoader < 0)
		RETURN_ERROR(fPackageLoader);

	// publish the root node
	fRootDirectory->AcquireReference();
	error = PublishVNode(fRootDirectory);
	if (error != B_OK) {
		fRootDirectory->ReleaseReference();
		return error;
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
Volume::PublishVNode(Node* node)
{
	return publish_vnode(fFSVolume, node->ID(), node, &gPackageFSVnodeOps,
		node->Mode() & S_IFMT, 0);
}


status_t
Volume::AddPackageDomain(const char* path)
{
	PackageDomain* packageDomain = new(std::nothrow) PackageDomain;
	if (packageDomain == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<PackageDomain> packageDomainDeleter(packageDomain);

	status_t error = packageDomain->Init(path);
	if (error != B_OK)
		return error;

	Job* job = new(std::nothrow) AddPackageDomainJob(this, packageDomain);
	if (job == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	packageDomainDeleter.Detach();

	_PushJob(job);

	return B_OK;
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
}


status_t
Volume::_AddInitialPackageDomain(const char* path)
{
	PackageDomain* domain = new(std::nothrow) PackageDomain;
	if (domain == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<PackageDomain> domainDeleter(domain);

	status_t error = domain->Init(path);
	if (error != B_OK)
		return error;

	return _AddPackageDomain(domainDeleter.Detach());
}


status_t
Volume::_AddPackageDomain(PackageDomain* domain)
{
	ObjectDeleter<PackageDomain> domainDeleter(domain);

	// prepare the package domain
	status_t error = domain->Prepare();
	if (error != B_OK) {
		ERROR("Failed to prepare package domain \"%s\": %s\n",
			domain->Path(), strerror(errno));
		return errno;
	}

	// TODO: Start monitoring the directory!

	// iterate through the dir and create packages
	DIR* dir = opendir(domain->Path());
	if (dir == NULL) {
		ERROR("Failed to open package domain directory \"%s\": %s\n",
			domain->Path(), strerror(errno));
		return errno;
	}
	CObjectDeleter<DIR, int> dirCloser(dir, closedir);

	while (dirent* entry = readdir(dir)) {
		// skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		// check whether the entry is a file
		struct stat st;
		if (fstatat(domain->DirectoryFD(), entry->d_name, &st,
				AT_SYMLINK_NOFOLLOW) < 0
			|| !S_ISREG(st.st_mode)) {
			continue;
		}

		// create a package
		Package* package = new(std::nothrow) Package(domain, st.st_dev,
			st.st_ino);
		if (package == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		ObjectDeleter<Package> packageDeleter(package);

		status_t error = package->Init(entry->d_name);
		if (error != B_OK)
			return error;

		error = _LoadPackage(package);
		if (error != B_OK)
			continue;

		domain->AddPackage(packageDeleter.Detach());
	}

	// add the packages to the node tree
	VolumeWriteLocker volumeLocker(this);
	for (PackageHashTable::Iterator it = domain->Packages().GetIterator();
			Package* package = it.Next();) {
		error = _AddPackageContent(package);
		if (error != B_OK) {
// TODO: Remove the already added packages!
			return error;
		}
	}

	fPackageDomains.Add(domainDeleter.Detach());
	return B_OK;
}


status_t
Volume::_LoadPackage(Package* package)
{
	// open package file
	int fd = openat(package->Domain()->DirectoryFD(), package->Name(),
		O_RDONLY);
	if (fd < 0) {
		ERROR("Failed to open package file \"%s\"\n", package->Name());
		return errno;
	}
// TODO: Verify that it's still the same file.
	FDCloser fdCloser(fd);

	// open package
	PackageLoaderErrorOutput errorOutput(package);
	PackageReader packageReader(&errorOutput);
	status_t error = packageReader.Init(fd, false);
	if (error != B_OK)
		return error;

	// parse content
	PackageLoaderContentHandler handler(package);
	error = handler.Init();
	if (error != B_OK)
		return error;

	error = packageReader.ParseContent(&handler);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
Volume::_AddPackageContent(Package* package)
{
	for (PackageNodeList::Iterator it = package->Nodes().GetIterator();
			PackageNode* node = it.Next();) {
		status_t error = _AddPackageContentRootNode(package, node);
		if (error != B_OK) {
// TODO: Remove already added nodes!
			return error;
		}
	}
// TODO: Recursively add the nodes!

	return B_OK;
}


status_t
Volume::_AddPackageContentRootNode(Package* package, PackageNode* packageNode)
{
	Directory* directory = fRootDirectory;
	directory->WriteLock();

	do {
		Node* node;
		status_t error = _AddPackageNode(directory, packageNode, node);
		if (error != B_OK) {
// TODO: Remove already added nodes!
			return error;
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

		// continue with the next available (descendent's) sibling
		do {
			PackageDirectory* packageDirectory = packageNode->Parent();
			if (PackageNode* sibling
					= packageDirectory->NextChild(packageNode)) {
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


status_t
Volume::_AddPackageNode(Directory* directory, PackageNode* packageNode,
	Node*& _node)
{
	Node* node = directory->FindChild(packageNode->Name());
	if (node == NULL) {
		status_t error = _CreateNode(packageNode->Mode(), directory,
			packageNode->Name(), node);
		if (error != B_OK)
			return error;
	}

	status_t error = node->AddPackageNode(packageNode);
	if (error != B_OK)
		return error;
// TODO: Remove the node, if created before!

	_node = node;
	return B_OK;
}


status_t
Volume::_CreateNode(mode_t mode, Directory* parent, const char* name,
	Node*& _node)
{
	Node* node;
	if (S_ISREG(mode) || S_ISLNK(mode))
		node = new(std::nothrow) LeafNode(fNextNodeID++);
	else if (S_ISDIR(mode))
		node = new(std::nothrow) Directory(fNextNodeID++);
	else
		RETURN_ERROR(B_UNSUPPORTED);

	if (node == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<Node> nodeDeleter(node);

	status_t error = node->Init(parent, name);
	if (error != B_OK)
		return error;

	parent->AddChild(node);

	fNodes.Insert(node);
		// we keep the initial node reference for this table

	_node = nodeDeleter.Detach();
	return B_OK;
}
