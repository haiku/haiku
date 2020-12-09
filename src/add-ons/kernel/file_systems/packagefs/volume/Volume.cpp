/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <new>

#include <AppDefs.h>
#include <driver_settings.h>
#include <KernelExport.h>
#include <NodeMonitor.h>
#include <package/PackageInfoAttributes.h>

#include <AutoDeleter.h>
#include <AutoDeleterPosix.h>
#include <AutoDeleterDrivers.h>
#include <PackagesDirectoryDefs.h>

#include <vfs.h>

#include "AttributeIndex.h"
#include "DebugSupport.h"
#include "kernel_interface.h"
#include "LastModifiedIndex.h"
#include "NameIndex.h"
#include "OldUnpackingNodeAttributes.h"
#include "PackageFSRoot.h"
#include "PackageLinkDirectory.h"
#include "PackageLinksDirectory.h"
#include "Resolvable.h"
#include "SizeIndex.h"
#include "UnpackingLeafNode.h"
#include "UnpackingDirectory.h"
#include "Utils.h"
#include "Version.h"


// node ID of the root directory
static const ino_t kRootDirectoryID = 1;

static const uint32 kAllStatFields = B_STAT_MODE | B_STAT_UID | B_STAT_GID
	| B_STAT_SIZE | B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME
	| B_STAT_CREATION_TIME | B_STAT_CHANGE_TIME;

// shine-through directories
const char* const kShineThroughDirectories[] = {
	"cache", "non-packaged", "packages", "settings", "var", NULL
};

// sanity limit for activation change request
const size_t kMaxActivationRequestSize = 10 * 1024 * 1024;

// sanity limit for activation file size
const size_t kMaxActivationFileSize = 10 * 1024 * 1024;

static const char* const kAdministrativeDirectoryName
	= PACKAGES_DIRECTORY_ADMIN_DIRECTORY;
static const char* const kActivationFileName
	= PACKAGES_DIRECTORY_ACTIVATION_FILE;
static const char* const kActivationFilePath
	= PACKAGES_DIRECTORY_ADMIN_DIRECTORY "/"
		PACKAGES_DIRECTORY_ACTIVATION_FILE;


// #pragma mark - ShineThroughDirectory


struct Volume::ShineThroughDirectory : public Directory {
	ShineThroughDirectory(ino_t id)
		:
		Directory(id)
	{
		get_real_time(fModifiedTime);
	}

	virtual timespec ModifiedTime() const
	{
		return fModifiedTime;
	}

private:
	timespec	fModifiedTime;
};


// #pragma mark - ActivationChangeRequest


struct Volume::ActivationChangeRequest {
public:
	ActivationChangeRequest()
		:
		fRequest(NULL),
		fRequestSize(0)
	{
	}

	~ActivationChangeRequest()
	{
		free(fRequest);
	}

	status_t Init(const void* userRequest, size_t requestSize)
	{
		// copy request to kernel
		if (requestSize > kMaxActivationRequestSize)
			RETURN_ERROR(B_BAD_VALUE);

		fRequest = (PackageFSActivationChangeRequest*)malloc(requestSize);
		if (fRequest == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		fRequestSize = requestSize;

		status_t error = user_memcpy(fRequest, userRequest, fRequestSize);
		if (error != B_OK)
			RETURN_ERROR(error);

		uint32 itemCount = fRequest->itemCount;
		const char* requestEnd = (const char*)fRequest + requestSize;
		if (&fRequest->items[itemCount] > (void*)requestEnd)
			RETURN_ERROR(B_BAD_VALUE);

		// adjust the item name pointers and check their validity
		addr_t nameDelta = (addr_t)fRequest - (addr_t)userRequest;
		for (uint32 i = 0; i < itemCount; i++) {
			PackageFSActivationChangeItem& item = fRequest->items[i];
			item.name += nameDelta;
			if (item.name < (char*)fRequest || item.name >= requestEnd)
				RETURN_ERROR(B_BAD_VALUE);
			size_t maxNameSize = requestEnd - item.name;
			if (strnlen(item.name, maxNameSize) == maxNameSize)
				RETURN_ERROR(B_BAD_VALUE);
		}

		return B_OK;
	}

	uint32 CountItems() const
	{
		return fRequest->itemCount;
	}

	PackageFSActivationChangeItem* ItemAt(uint32 index) const
	{
		return index < CountItems() ? &fRequest->items[index] : NULL;
	}

private:
	PackageFSActivationChangeRequest*	fRequest;
	size_t								fRequestSize;
};


// #pragma mark - Volume


Volume::Volume(fs_volume* fsVolume)
	:
	fFSVolume(fsVolume),
	fRootDirectory(NULL),
	fPackageFSRoot(NULL),
	fPackagesDirectory(NULL),
	fPackagesDirectories(),
	fPackagesDirectoriesByNodeRef(),
	fPackageSettings(),
	fNextNodeID(kRootDirectoryID + 1)
{
	rw_lock_init(&fLock, "packagefs volume");
}


Volume::~Volume()
{
	// remove the packages from the node tree
	{
		VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
		VolumeWriteLocker volumeLocker(this);
		for (PackageFileNameHashTable::Iterator it = fPackages.GetIterator();
			Package* package = it.Next();) {
			_RemovePackageContent(package, NULL, false);
		}
	}

	// delete the packages
	_RemoveAllPackages();

	// delete all indices
	Index* index = fIndices.Clear(true);
	while (index != NULL) {
		Index* next = index->IndexHashLink();
		delete index;
		index = next;
	}

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

	while (PackagesDirectory* directory = fPackagesDirectories.RemoveHead())
		directory->ReleaseReference();

	rw_lock_destroy(&fLock);
}


status_t
Volume::Mount(const char* parameterString)
{
	// init the hash tables
	status_t error = fPackagesDirectoriesByNodeRef.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = fNodes.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = fNodeListeners.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = fPackages.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = fIndices.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the name index
	{
		NameIndex* index = new(std::nothrow) NameIndex;
		if (index == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		error = index->Init(this);
		if (error != B_OK) {
			delete index;
			RETURN_ERROR(error);
		}

		fIndices.Insert(index);
	}

	// create the size index
	{
		SizeIndex* index = new(std::nothrow) SizeIndex;
		if (index == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		error = index->Init(this);
		if (error != B_OK) {
			delete index;
			RETURN_ERROR(error);
		}

		fIndices.Insert(index);
	}

	// create the last modified index
	{
		LastModifiedIndex* index = new(std::nothrow) LastModifiedIndex;
		if (index == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		error = index->Init(this);
		if (error != B_OK) {
			delete index;
			RETURN_ERROR(error);
		}

		fIndices.Insert(index);
	}

	// create a BEOS:APP_SIG index
	{
		AttributeIndex* index = new(std::nothrow) AttributeIndex;
		if (index == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		error = index->Init(this, "BEOS:APP_SIG", B_MIME_STRING_TYPE, 0);
		if (error != B_OK) {
			delete index;
			RETURN_ERROR(error);
		}

		fIndices.Insert(index);
	}

	// get the mount parameters
	const char* packages = NULL;
	const char* volumeName = NULL;
	const char* mountType = NULL;
	const char* shineThrough = NULL;
	const char* packagesState = NULL;

	void* parameterHandle = parse_driver_settings_string(parameterString);
	if (parameterHandle != NULL) {
		packages = get_driver_parameter(parameterHandle, "packages", NULL,
			NULL);
		volumeName = get_driver_parameter(parameterHandle, "volume-name", NULL,
			NULL);
		mountType = get_driver_parameter(parameterHandle, "type", NULL, NULL);
		shineThrough = get_driver_parameter(parameterHandle, "shine-through",
			NULL, NULL);
		packagesState = get_driver_parameter(parameterHandle, "state", NULL,
			NULL);
	}

	DriverSettingsUnloader parameterHandleDeleter(parameterHandle);

	if (packages != NULL && packages[0] == '\0') {
		FATAL("invalid package folder ('packages' parameter)!\n");
		RETURN_ERROR(B_BAD_VALUE);
	}

	error = _InitMountType(mountType);
	if (error != B_OK) {
		FATAL("invalid mount type: \"%s\"\n", mountType);
		RETURN_ERROR(error);
	}

	// get our mount point
	error = vfs_get_mount_point(fFSVolume->id, &fMountPoint.deviceID,
		&fMountPoint.nodeID);
	if (error != B_OK)
		RETURN_ERROR(error);

	// load package settings
	error = fPackageSettings.Load(fMountPoint.deviceID, fMountPoint.nodeID,
		fMountType);
	// abort only in case of serious issues (memory shortage)
	if (error == B_NO_MEMORY)
		RETURN_ERROR(error);

	// create package domain
	fPackagesDirectory = new(std::nothrow) PackagesDirectory;
	if (fPackagesDirectory == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	fPackagesDirectories.Add(fPackagesDirectory);
	fPackagesDirectoriesByNodeRef.Insert(fPackagesDirectory);

	struct stat st;
	error = fPackagesDirectory->Init(packages, fMountPoint.deviceID,
		fMountPoint.nodeID, st);
	if (error != B_OK)
		RETURN_ERROR(error);

	// If a packages state has been specified, load the needed states.
	if (packagesState != NULL) {
		error = _LoadOldPackagesStates(packagesState);
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	// If no volume name is given, infer it from the mount type.
	if (volumeName == NULL) {
		switch (fMountType) {
			case PACKAGE_FS_MOUNT_TYPE_SYSTEM:
				volumeName = "system";
				break;
			case PACKAGE_FS_MOUNT_TYPE_HOME:
				volumeName = "config";
				break;
			case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
			default:
				volumeName = "Package FS";
				break;
		}
	}

	String volumeNameString;
	if (!volumeNameString.SetTo(volumeName))
		RETURN_ERROR(B_NO_MEMORY);

	// create the root node
	fRootDirectory
		= new(std::nothrow) ::RootDirectory(kRootDirectoryID, st.st_mtim);
	if (fRootDirectory == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	fRootDirectory->Init(NULL, volumeNameString);
	fNodes.Insert(fRootDirectory);
	fRootDirectory->AcquireReference();
		// one reference for the table

	// register with packagefs root
	error = ::PackageFSRoot::RegisterVolume(this);
	if (error != B_OK)
		RETURN_ERROR(error);

	if (this == fPackageFSRoot->SystemVolume()) {
		error = _AddPackageLinksDirectory();
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	// create shine-through directories
	error = _CreateShineThroughDirectories(shineThrough);
	if (error != B_OK)
		RETURN_ERROR(error);

	// add initial packages
	error = _AddInitialPackages();
	if (error != B_OK)
		RETURN_ERROR(error);

	// publish the root node
	fRootDirectory->AcquireReference();
	error = PublishVNode(fRootDirectory);
	if (error != B_OK) {
		fRootDirectory->ReleaseReference();
		RETURN_ERROR(error);
	}

	// bind and publish the shine-through directories
	error = _PublishShineThroughDirectories();
	if (error != B_OK)
		RETURN_ERROR(error);

	StringPool::DumpUsageStatistics();

	return B_OK;
}


void
Volume::Unmount()
{
}


status_t
Volume::IOCtl(Node* node, uint32 operation, void* buffer, size_t size)
{
	switch (operation) {
		case PACKAGE_FS_OPERATION_GET_VOLUME_INFO:
		{
			if (size < sizeof(PackageFSVolumeInfo))
				RETURN_ERROR(B_BAD_VALUE);

			PackageFSVolumeInfo* userVolumeInfo
				= (PackageFSVolumeInfo*)buffer;

			VolumeReadLocker volumeReadLocker(this);

			PackageFSVolumeInfo volumeInfo;
			volumeInfo.mountType = fMountType;
			volumeInfo.rootDeviceID = fPackageFSRoot->DeviceID();
			volumeInfo.rootDirectoryID = fPackageFSRoot->NodeID();
			volumeInfo.packagesDirectoryCount = fPackagesDirectories.Count();

			status_t error = user_memcpy(userVolumeInfo, &volumeInfo,
				sizeof(volumeInfo));
			if (error != B_OK)
				RETURN_ERROR(error);

			uint32 directoryIndex = 0;
			for (PackagesDirectoryList::Iterator it
					= fPackagesDirectories.GetIterator();
				PackagesDirectory* directory = it.Next();
				directoryIndex++) {
				PackageFSDirectoryInfo info;
				info.deviceID = directory->DeviceID();
				info.nodeID = directory->NodeID();

				PackageFSDirectoryInfo* userInfo
					= userVolumeInfo->packagesDirectoryInfos + directoryIndex;
				if (addr_t(userInfo + 1) > (addr_t)buffer + size)
					break;

				if (user_memcpy(userInfo, &info, sizeof(info)) != B_OK)
					return B_BAD_ADDRESS;
			}

			return B_OK;
		}

		case PACKAGE_FS_OPERATION_GET_PACKAGE_INFOS:
		{
			if (size < sizeof(PackageFSGetPackageInfosRequest))
				RETURN_ERROR(B_BAD_VALUE);

			PackageFSGetPackageInfosRequest* request
				= (PackageFSGetPackageInfosRequest*)buffer;

			VolumeReadLocker volumeReadLocker(this);

			addr_t bufferEnd = (addr_t)buffer + size;
			uint32 packageCount = fPackages.CountElements();
			char* nameBuffer = (char*)(request->infos + packageCount);

			uint32 packageIndex = 0;
			for (PackageFileNameHashTable::Iterator it
					= fPackages.GetIterator(); it.HasNext();
				packageIndex++) {
				Package* package = it.Next();
				PackageFSPackageInfo info;
				info.packageDeviceID = package->DeviceID();
				info.packageNodeID = package->NodeID();
				PackagesDirectory* directory = package->Directory();
				info.directoryDeviceID = directory->DeviceID();
				info.directoryNodeID = directory->NodeID();
				info.name = nameBuffer;

				PackageFSPackageInfo* userInfo = request->infos + packageIndex;
				if (addr_t(userInfo + 1) <= bufferEnd) {
					if (user_memcpy(userInfo, &info, sizeof(info)) != B_OK)
						return B_BAD_ADDRESS;
				}

				const char* name = package->FileName();
				size_t nameSize = strlen(name) + 1;
				char* nameEnd = nameBuffer + nameSize;
				if ((addr_t)nameEnd <= bufferEnd) {
					if (user_memcpy(nameBuffer, name, nameSize) != B_OK)
						return B_BAD_ADDRESS;
				}
				nameBuffer = nameEnd;
			}

			PackageFSGetPackageInfosRequest header;
			header.bufferSize = nameBuffer - (char*)request;
			header.packageCount = packageCount;
			size_t headerSize = (char*)&request->infos - (char*)request;
			RETURN_ERROR(user_memcpy(request, &header, headerSize));
		}

		case PACKAGE_FS_OPERATION_CHANGE_ACTIVATION:
		{
			ActivationChangeRequest request;
			status_t error = request.Init(buffer, size);
			if (error != B_OK)
				RETURN_ERROR(B_BAD_VALUE);

			return _ChangeActivation(request);
		}

		default:
			return B_BAD_VALUE;
	}
}


void
Volume::AddNodeListener(NodeListener* listener, Node* node)
{
	ASSERT(!listener->IsListening());

	listener->StartedListening(node);

	if (NodeListener* list = fNodeListeners.Lookup(node))
		list->AddNodeListener(listener);
	else
		fNodeListeners.Insert(listener);
}


void
Volume::RemoveNodeListener(NodeListener* listener)
{
	ASSERT(listener->IsListening());

	Node* node = listener->ListenedNode();

	if (NodeListener* next = listener->RemoveNodeListener()) {
		// list not empty yet -- if we removed the head, add a new head to the
		// hash table
		NodeListener* list = fNodeListeners.Lookup(node);
		if (list == listener) {
			fNodeListeners.Remove(listener);
			fNodeListeners.Insert(next);
		}
	} else
		fNodeListeners.Remove(listener);

	listener->StoppedListening();
}


void
Volume::AddQuery(Query* query)
{
	fQueries.Add(query);
}


void
Volume::RemoveQuery(Query* query)
{
	fQueries.Remove(query);
}


void
Volume::UpdateLiveQueries(Node* node, const char* attribute, int32 type,
	const void* oldKey, size_t oldLength, const void* newKey,
	size_t newLength)
{
	for (QueryList::Iterator it = fQueries.GetIterator();
			Query* query = it.Next();) {
		query->LiveUpdate(node, attribute, type, oldKey, oldLength, newKey,
			newLength);
	}
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


void
Volume::PackageLinkNodeAdded(Node* node)
{
	_AddPackageLinksNode(node);

	notify_entry_created(ID(), node->Parent()->ID(), node->Name(), node->ID());
	_NotifyNodeAdded(node);
}


void
Volume::PackageLinkNodeRemoved(Node* node)
{
	_RemovePackageLinksNode(node);

	notify_entry_removed(ID(), node->Parent()->ID(), node->Name(), node->ID());
	_NotifyNodeRemoved(node);
}


void
Volume::PackageLinkNodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	Directory* parent = node->Parent();
	notify_stat_changed(ID(), parent != NULL ? parent->ID() : -1, node->ID(),
		statFields);
	_NotifyNodeChanged(node, statFields, oldAttributes);
}


status_t
Volume::_LoadOldPackagesStates(const char* packagesState)
{
	// open and stat the admininistrative dir
	int fd = openat(fPackagesDirectory->DirectoryFD(),
		kAdministrativeDirectoryName, O_RDONLY);
	if (fd < 0) {
		ERROR("Failed to open administrative directory: %s\n", strerror(errno));
		RETURN_ERROR(errno);
	}

	struct stat adminDirStat;
	if (fstat(fd, &adminDirStat) < 0) {
		ERROR("Failed to fstat() administrative directory: %s\n",
			strerror(errno));
		RETURN_ERROR(errno);
	}

	// iterate through the "administrative" dir
	DIR* dir = fdopendir(fd);
	if (dir == NULL) {
		ERROR("Failed to open administrative directory: %s\n", strerror(errno));
		RETURN_ERROR(errno);
	}
	DirCloser dirCloser(dir);

	while (dirent* entry = readdir(dir)) {
		if (strncmp(entry->d_name, "state_", 6) != 0
			|| strcmp(entry->d_name, packagesState) < 0) {
			continue;
		}

		PackagesDirectory* packagesDirectory
			= new(std::nothrow) PackagesDirectory;
		status_t error = packagesDirectory->InitOldState(adminDirStat.st_dev,
			adminDirStat.st_ino, entry->d_name);
		if (error != B_OK) {
			delete packagesDirectory;
			continue;
		}

		fPackagesDirectories.Add(packagesDirectory);
		fPackagesDirectoriesByNodeRef.Insert(packagesDirectory);

		INFORM("added old packages dir state \"%s\"\n",
			packagesDirectory->StateName().Data());
	}

	// sort the packages directories by state age
	fPackagesDirectories.Sort(&PackagesDirectory::IsNewer);

	return B_OK;
}


status_t
Volume::_AddInitialPackages()
{
	PackagesDirectory* packagesDirectory = fPackagesDirectories.Last();
	INFORM("Adding packages from \"%s\"\n", packagesDirectory->Path());

	// try reading the activation file of the oldest state
	status_t error = _AddInitialPackagesFromActivationFile(packagesDirectory);
	if (error != B_OK && packagesDirectory != fPackagesDirectory) {
		WARN("Loading packages from old state \"%s\" failed. Loading packages "
			"from latest state.\n", packagesDirectory->StateName().Data());

		// remove all packages already added
		{
			VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
			VolumeWriteLocker volumeLocker(this);
			_RemoveAllPackages();
		}

		// remove the old states
		while (fPackagesDirectories.Last() != fPackagesDirectory)
			fPackagesDirectories.RemoveTail()->ReleaseReference();

		// try reading the activation file of the latest state
		packagesDirectory = fPackagesDirectory;
		error = _AddInitialPackagesFromActivationFile(packagesDirectory);
	}

	if (error != B_OK) {
		INFORM("Loading packages from activation file failed. Loading all "
			"packages in packages directory.\n");

		// remove all packages already added
		{
			VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
			VolumeWriteLocker volumeLocker(this);
			_RemoveAllPackages();
		}

		// read the whole directory
		error = _AddInitialPackagesFromDirectory();
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	// add the packages to the node tree
	VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
	VolumeWriteLocker volumeLocker(this);
	for (PackageFileNameHashTable::Iterator it = fPackages.GetIterator();
		Package* package = it.Next();) {
		error = _AddPackageContent(package, false);
		if (error != B_OK) {
			for (it.Rewind(); Package* activePackage = it.Next();) {
				if (activePackage == package)
					break;
				_RemovePackageContent(activePackage, NULL, false);
			}
			RETURN_ERROR(error);
		}
	}

	return B_OK;
}


status_t
Volume::_AddInitialPackagesFromActivationFile(
	PackagesDirectory* packagesDirectory)
{
	// try reading the activation file
	int fd = openat(packagesDirectory->DirectoryFD(),
		packagesDirectory == fPackagesDirectory
			? kActivationFilePath : kActivationFileName,
		O_RDONLY);
	if (fd < 0) {
		INFORM("Failed to open packages activation file: %s\n",
			strerror(errno));
		RETURN_ERROR(errno);
	}
	FileDescriptorCloser fdCloser(fd);

	// read the whole file into memory to simplify things
	struct stat st;
	if (fstat(fd, &st) != 0) {
		ERROR("Failed to stat packages activation file: %s\n",
			strerror(errno));
		RETURN_ERROR(errno);
	}

	if (st.st_size > (off_t)kMaxActivationFileSize) {
		ERROR("The packages activation file is too big.\n");
		RETURN_ERROR(B_BAD_DATA);
	}

	char* fileContent = (char*)malloc(st.st_size + 1);
	if (fileContent == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	MemoryDeleter fileContentDeleter(fileContent);

	ssize_t bytesRead = read(fd, fileContent, st.st_size);
	if (bytesRead < 0) {
		ERROR("Failed to read packages activation file: %s\n", strerror(errno));
		RETURN_ERROR(errno);
	}

	if (bytesRead != st.st_size) {
		ERROR("Failed to read whole packages activation file\n");
		RETURN_ERROR(B_ERROR);
	}

	// null-terminate to simplify parsing
	fileContent[st.st_size] = '\0';

	// parse the file and add the respective packages
	const char* packageName = fileContent;
	char* const fileContentEnd = fileContent + st.st_size;
	while (packageName < fileContentEnd) {
		char* packageNameEnd = strchr(packageName, '\n');
		if (packageNameEnd == NULL)
			packageNameEnd = fileContentEnd;

		// skip empty lines
		if (packageName == packageNameEnd) {
			packageName++;
			continue;
		}
		*packageNameEnd = '\0';

		if (packageNameEnd - packageName >= B_FILE_NAME_LENGTH) {
			ERROR("Invalid packages activation file content.\n");
			RETURN_ERROR(B_BAD_DATA);
		}

		status_t error = _LoadAndAddInitialPackage(packagesDirectory,
			packageName);
		if (error != B_OK)
			RETURN_ERROR(error);

		packageName = packageNameEnd + 1;
	}

	return B_OK;
}


status_t
Volume::_AddInitialPackagesFromDirectory()
{
	// iterate through the dir and create packages
	int fd = openat(fPackagesDirectory->DirectoryFD(), ".", O_RDONLY);
	if (fd < 0) {
		ERROR("Failed to open packages directory: %s\n", strerror(errno));
		RETURN_ERROR(errno);
	}

	DIR* dir = fdopendir(fd);
	if (dir == NULL) {
		ERROR("Failed to open packages directory \"%s\": %s\n",
			fPackagesDirectory->Path(), strerror(errno));
		RETURN_ERROR(errno);
	}
	DirCloser dirCloser(dir);

	while (dirent* entry = readdir(dir)) {
		// skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		// also skip any entry without a ".hpkg" extension
		size_t nameLength = strlen(entry->d_name);
		if (nameLength < 5
			|| memcmp(entry->d_name + nameLength - 5, ".hpkg", 5) != 0) {
			continue;
		}

		_LoadAndAddInitialPackage(fPackagesDirectory, entry->d_name);
	}

	return B_OK;
}


status_t
Volume::_LoadAndAddInitialPackage(PackagesDirectory* packagesDirectory,
	const char* name)
{
	Package* package;
	status_t error = _LoadPackage(packagesDirectory, name, package);
	if (error != B_OK) {
		ERROR("Failed to load package \"%s\": %s\n", name, strerror(error));
		RETURN_ERROR(error);
	}
	BReference<Package> packageReference(package, true);

	VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
	VolumeWriteLocker volumeLocker(this);
	_AddPackage(package);

	return B_OK;
}


inline void
Volume::_AddPackage(Package* package)
{
	fPackages.Insert(package);
	package->AcquireReference();
}


inline void
Volume::_RemovePackage(Package* package)
{
	fPackages.Remove(package);
	package->ReleaseReference();
}


void
Volume::_RemoveAllPackages()
{
	Package* package = fPackages.Clear(true);
	while (package != NULL) {
		Package* next = package->FileNameHashTableNext();
		package->ReleaseReference();
		package = next;
	}
}


inline Package*
Volume::_FindPackage(const char* fileName) const
{
	return fPackages.Lookup(fileName);
}


status_t
Volume::_AddPackageContent(Package* package, bool notify)
{
	// Open the package. We don't need the FD here, but this is an optimization.
	// The attribute indices may want to read the package nodes' attributes and
	// the package file would be opened and closed for each attribute instance.
	// Since Package keeps and shares the FD as long as at least one party has
	// the package open, we prevent that.
	int fd = package->Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(package);

	status_t error = fPackageFSRoot->AddPackage(package);
	if (error != B_OK)
		RETURN_ERROR(error);

	for (PackageNodeList::Iterator it = package->Nodes().GetIterator();
			PackageNode* node = it.Next();) {
		// skip over ".PackageInfo" file, it isn't part of the package content
		if (strcmp(node->Name(),
				BPackageKit::BHPKG::B_HPKG_PACKAGE_INFO_FILE_NAME) == 0) {
			continue;
		}
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
		if (strcmp(node->Name(),
				BPackageKit::BHPKG::B_HPKG_PACKAGE_INFO_FILE_NAME) != 0) {
			_RemovePackageContentRootNode(package, node, NULL, notify);
		}

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
			// returns B_OK with a NULL node, when skipping the node
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

		// recurse into directory, unless we're supposed to skip the node
		if (node != NULL) {
			if (PackageDirectory* packageDirectory
					= dynamic_cast<PackageDirectory*>(packageNode)) {
				if (packageDirectory->FirstChild() != NULL) {
					directory = dynamic_cast<Directory*>(node);
					packageNode = packageDirectory->FirstChild();
					directory->WriteLock();
					continue;
				}
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

		// recurse into directory
		if (PackageDirectory* packageDirectory
				= dynamic_cast<PackageDirectory*>(packageNode)) {
			if (packageDirectory->FirstChild() != NULL) {
				if (Directory* childDirectory = dynamic_cast<Directory*>(
						directory->FindChild(packageNode->Name()))) {
					directory = childDirectory;
					packageNode = packageDirectory->FirstChild();
					directory->WriteLock();
					continue;
				}
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
	PackageNode* oldPackageNode = NULL;

	if (node != NULL) {
		unpackingNode = dynamic_cast<UnpackingNode*>(node);
		if (unpackingNode == NULL) {
			_node = NULL;
			return B_OK;
		}
		oldPackageNode = unpackingNode->GetPackageNode();
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

	BReference<Node> newNodeReference;
	NodeWriteLocker newNodeWriteLocker;
	Node* oldNode = NULL;

	if (!newNode && !S_ISDIR(node->Mode()) && oldPackageNode != NULL
		&& unpackingNode->WillBeFirstPackageNode(packageNode)) {
		// The package node we're going to add will represent the node,
		// replacing the current head package node. Since the node isn't a
		// directory, we must make sure that clients having opened or mapped the
		// node won't be surprised. So we create a new node and remove the
		// current one.
		// create a new node and transfer the package nodes to it
		UnpackingNode* newUnpackingNode;
		status_t error = unpackingNode->CloneTransferPackageNodes(
			fNextNodeID++, newUnpackingNode);
		if (error != B_OK)
			RETURN_ERROR(error);

		// remove the old node
		_NotifyNodeRemoved(node);
		_RemoveNodeAndVNode(node);
		oldNode = node;

		// add the new node
		unpackingNode = newUnpackingNode;
		node = unpackingNode->GetNode();
		newNodeReference.SetTo(node);
		newNodeWriteLocker.SetTo(node, false);

		directory->AddChild(node);
		fNodes.Insert(node);
		newNode = true;
	}

	status_t error = unpackingNode->AddPackageNode(packageNode, ID());
	if (error != B_OK) {
		// Remove the node, if created before. If the node was created to
		// replace the previous node, send out notifications instead.
		if (newNode) {
			if (oldNode != NULL) {
				_NotifyNodeAdded(node);
				if (notify) {
					notify_entry_removed(ID(), directory->ID(), oldNode->Name(),
						oldNode->ID());
					notify_entry_created(ID(), directory->ID(), node->Name(),
						node->ID());
				}
			} else
				_RemoveNode(node);
		}
		RETURN_ERROR(error);
	}

	if (newNode) {
		_NotifyNodeAdded(node);
	} else if (packageNode == unpackingNode->GetPackageNode()) {
		_NotifyNodeChanged(node, kAllStatFields,
			OldUnpackingNodeAttributes(oldPackageNode));
	}

	if (notify) {
		if (newNode) {
			if (oldNode != NULL) {
				notify_entry_removed(ID(), directory->ID(), oldNode->Name(),
					oldNode->ID());
			}
			notify_entry_created(ID(), directory->ID(), node->Name(),
				node->ID());
		} else if (packageNode == unpackingNode->GetPackageNode()) {
			// The new package node has become the one representing the node.
			// Send stat changed notification for directories and entry
			// removed + created notifications for files and symlinks.
			notify_stat_changed(ID(), directory->ID(), node->ID(),
				kAllStatFields);
			// TODO: Actually the attributes might change, too!
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
	bool nodeRemoved = false;
	Node* newNode = NULL;

	BReference<Node> newNodeReference;
	NodeWriteLocker newNodeWriteLocker;

	// If this is the last package node of the node, remove it completely.
	if (unpackingNode->IsOnlyPackageNode(packageNode)) {
		// Notify before removing the node. Otherwise the indices might not
		// find the node anymore.
		_NotifyNodeRemoved(node);

		unpackingNode->PrepareForRemoval();

		_RemoveNodeAndVNode(node);
		nodeRemoved = true;
	} else if (packageNode == headPackageNode) {
		// The node does at least have one more package node, but the one to be
		// removed is the head. Unless it's a directory, we replace the node
		// with a completely new one and let the old one die. This is necessary
		// to avoid surprises for clients that have opened/mapped the node.
		if (S_ISDIR(packageNode->Mode())) {
			unpackingNode->RemovePackageNode(packageNode, ID());
			_NotifyNodeChanged(node, kAllStatFields,
				OldUnpackingNodeAttributes(headPackageNode));
		} else {
			// create a new node and transfer the package nodes to it
			UnpackingNode* newUnpackingNode;
			status_t error = unpackingNode->CloneTransferPackageNodes(
				fNextNodeID++, newUnpackingNode);
			if (error == B_OK) {
				// remove the package node
				newUnpackingNode->RemovePackageNode(packageNode, ID());

				// remove the old node
				_NotifyNodeRemoved(node);
				_RemoveNodeAndVNode(node);

				// add the new node
				newNode = newUnpackingNode->GetNode();
				newNodeReference.SetTo(newNode);
				newNodeWriteLocker.SetTo(newNode, false);

				directory->AddChild(newNode);
				fNodes.Insert(newNode);
				_NotifyNodeAdded(newNode);
			} else {
				// There's nothing we can do. Remove the node completely.
				_NotifyNodeRemoved(node);

				unpackingNode->PrepareForRemoval();

				_RemoveNodeAndVNode(node);
				nodeRemoved = true;
			}
		}
	} else {
		// The package node to remove is not the head of the node. This change
		// doesn't have any visible effect.
		unpackingNode->RemovePackageNode(packageNode, ID());
	}

	if (!notify)
		return;

	// send notifications
	if (nodeRemoved) {
		notify_entry_removed(ID(), directory->ID(), node->Name(), node->ID());
	} else if (packageNode == headPackageNode) {
		// The removed package node was the one representing the node.
		// Send stat changed notification for directories and entry
		// removed + created notifications for files and symlinks.
		if (S_ISDIR(packageNode->Mode())) {
			notify_stat_changed(ID(), directory->ID(), node->ID(),
				kAllStatFields);
			// TODO: Actually the attributes might change, too!
		} else {
			notify_entry_removed(ID(), directory->ID(), node->Name(),
				node->ID());
			notify_entry_created(ID(), directory->ID(), newNode->Name(),
				newNode->ID());
		}
	}
}


status_t
Volume::_CreateUnpackingNode(mode_t mode, Directory* parent, const String& name,
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

	status_t error = node->Init(parent, name);
	if (error != B_OK)
		RETURN_ERROR(error);

	parent->AddChild(node);

	fNodes.Insert(node);
	nodeReference.Detach();
		// we keep the initial node reference for the table

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
Volume::_RemoveNodeAndVNode(Node* node)
{
	// If the node is known to the VFS, we get the vnode, remove it, and put it,
	// so that the VFS will discard it as soon as possible (i.e. now, if no one
	// else is using it).
	NodeWriteLocker nodeWriteLocker(node);

	// Remove the node from its parent and the volume. This makes the node
	// inaccessible via the get_vnode() and lookup() hooks.
	_RemoveNode(node);

	bool getVNode = node->IsKnownToVFS();

	nodeWriteLocker.Unlock();

	// Get a vnode reference, if the node is already known to the VFS.
	Node* dummyNode;
	if (getVNode && GetVNode(node->ID(), dummyNode) == B_OK) {
		// TODO: There still is a race condition here which we can't avoid
		// without more help from the VFS. Right after we drop the write
		// lock a vnode for the node could be discarded by the VFS. At that
		// point another thread trying to get the vnode by ID would create
		// a vnode, mark it busy and call our get_vnode() hook. It would
		// block since we (i.e. the package loader thread executing this
		// method) still have the volume write lock. Our get_vnode() call
		// would block, since it finds the vnode marked busy. It times out
		// eventually, but until then a good deal of FS operations might
		// block as well due to us holding the volume lock and probably
		// several node locks as well. A get_vnode*() variant (e.g.
		// get_vnode_etc() with flags parameter) that wouldn't block and
		// only get the vnode, if already loaded and non-busy, would be
		// perfect here.
		RemoveVNode(node->ID());
		PutVNode(node->ID());
	}
}


status_t
Volume::_LoadPackage(PackagesDirectory* packagesDirectory, const char* name,
	Package*& _package)
{
	// Find the package -- check the specified packages directory and iterate
	// toward the newer states.
	struct stat st;
	for (;;) {
		if (packagesDirectory == NULL)
			return B_ENTRY_NOT_FOUND;

		if (fstatat(packagesDirectory->DirectoryFD(), name, &st, 0) == 0) {
			// check whether the entry is a file
			if (!S_ISREG(st.st_mode))
				return B_BAD_VALUE;
			break;
		}

		packagesDirectory = fPackagesDirectories.GetPrevious(packagesDirectory);
	}

	// create a package
	Package* package = new(std::nothrow) Package(this, packagesDirectory,
		st.st_dev, st.st_ino);
	if (package == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	BReference<Package> packageReference(package, true);

	status_t error = package->Init(name);
	if (error != B_OK)
		return error;

	error = package->Load(fPackageSettings);
	if (error != B_OK)
		return error;

	_package = packageReference.Detach();
	return B_OK;
}


status_t
Volume::_ChangeActivation(ActivationChangeRequest& request)
{
	uint32 itemCount = request.CountItems();
	if (itemCount == 0)
		return B_OK;

	// first check the request
	int32 newPackageCount = 0;
	int32 oldPackageCount = 0;
	{
		VolumeReadLocker volumeLocker(this);

		for (uint32 i = 0; i < itemCount; i++) {
			PackageFSActivationChangeItem* item = request.ItemAt(i);
			if (item->parentDeviceID != fPackagesDirectory->DeviceID()
				|| item->parentDirectoryID != fPackagesDirectory->NodeID()) {
				ERROR("Volume::_ChangeActivation(): mismatching packages "
					"directory\n");
				RETURN_ERROR(B_MISMATCHED_VALUES);
			}

			Package* package = _FindPackage(item->name);
// TODO: We should better look up the package by node_ref!
			if (item->type == PACKAGE_FS_ACTIVATE_PACKAGE) {
				if (package != NULL) {
					ERROR("Volume::_ChangeActivation(): package to activate "
						"already activated: \"%s\"\n", item->name);
					RETURN_ERROR(B_NAME_IN_USE);
				}
				newPackageCount++;
			} else if (item->type == PACKAGE_FS_DEACTIVATE_PACKAGE) {
				if (package == NULL) {
					ERROR("Volume::_ChangeActivation(): package to deactivate "
						"not found: \"%s\"\n", item->name);
					RETURN_ERROR(B_NAME_NOT_FOUND);
				}
				oldPackageCount++;
			} else if (item->type == PACKAGE_FS_REACTIVATE_PACKAGE) {
				if (package == NULL) {
					ERROR("Volume::_ChangeActivation(): package to reactivate "
						"not found: \"%s\"\n", item->name);
					RETURN_ERROR(B_NAME_NOT_FOUND);
				}
				oldPackageCount++;
				newPackageCount++;
			} else
				RETURN_ERROR(B_BAD_VALUE);
		}
	}

	INFORM("Volume::_ChangeActivation(): %" B_PRId32 " new packages, %" B_PRId32
		" old packages\n", newPackageCount, oldPackageCount);

	// Things look good so far -- allocate reference arrays for the packages to
	// add and remove.
	BReference<Package>* newPackageReferences
		= new(std::nothrow) BReference<Package>[newPackageCount];
	if (newPackageReferences == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ArrayDeleter<BReference<Package> > newPackageReferencesDeleter(
			newPackageReferences);

	BReference<Package>* oldPackageReferences
		= new(std::nothrow) BReference<Package>[oldPackageCount];
	if (oldPackageReferences == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ArrayDeleter<BReference<Package> > oldPackageReferencesDeleter(
			oldPackageReferences);

	// load all new packages
	int32 newPackageIndex = 0;
	for (uint32 i = 0; i < itemCount; i++) {
		PackageFSActivationChangeItem* item = request.ItemAt(i);

		if (item->type != PACKAGE_FS_ACTIVATE_PACKAGE
			&& item->type != PACKAGE_FS_REACTIVATE_PACKAGE) {
			continue;
		}

		Package* package;
		status_t error = _LoadPackage(fPackagesDirectory, item->name, package);
		if (error != B_OK) {
			ERROR("Volume::_ChangeActivation(): failed to load package "
				"\"%s\"\n", item->name);
			RETURN_ERROR(error);
		}

		newPackageReferences[newPackageIndex++].SetTo(package, true);
	}

	// apply the changes
	VolumeWriteLocker systemVolumeLocker(_SystemVolumeIfNotSelf());
	VolumeWriteLocker volumeLocker(this);
// TODO: Add a change counter to Volume, so we can easily check whether
// everything is still the same.

	// remove the old packages
	int32 oldPackageIndex = 0;
	for (uint32 i = 0; i < itemCount; i++) {
		PackageFSActivationChangeItem* item = request.ItemAt(i);

		if (item->type != PACKAGE_FS_DEACTIVATE_PACKAGE
			&& item->type != PACKAGE_FS_REACTIVATE_PACKAGE) {
			continue;
		}

		Package* package = _FindPackage(item->name);
// TODO: We should better look up the package by node_ref!
		oldPackageReferences[oldPackageIndex++].SetTo(package);
		_RemovePackageContent(package, NULL, true);
		_RemovePackage(package);

		INFORM("package \"%s\" deactivated\n", package->FileName().Data());
	}
// TODO: Since package removal cannot fail, consider adding the new packages
// first. The reactivation case may make that problematic, since two packages
// with the same name would be active after activating the new one. Check!

	// add the new packages
	status_t error = B_OK;
	for (newPackageIndex = 0; newPackageIndex < newPackageCount;
		newPackageIndex++) {
		Package* package = newPackageReferences[newPackageIndex];
		_AddPackage(package);

		// add the package to the node tree
		error = _AddPackageContent(package, true);
		if (error != B_OK) {
			_RemovePackage(package);
			break;
		}
		INFORM("package \"%s\" activated\n", package->FileName().Data());
	}

	// Try to roll back the changes, if an error occurred.
	if (error != B_OK) {
		for (int32 i = newPackageIndex - 1; i >= 0; i--) {
			Package* package = newPackageReferences[i];
			_RemovePackageContent(package, NULL, true);
			_RemovePackage(package);
		}

		for (int32 i = oldPackageCount - 1; i >= 0; i--) {
			Package* package = oldPackageReferences[i];
			_AddPackage(package);

			if (_AddPackageContent(package, true) != B_OK) {
				// nothing we can do here
				ERROR("Volume::_ChangeActivation(): failed to roll back "
					"deactivation of package \"%s\" after error\n",
		  			package->FileName().Data());
				_RemovePackage(package);
			}
		}
	}

	return error;
}


status_t
Volume::_InitMountType(const char* mountType)
{
	if (mountType == NULL)
		fMountType = PACKAGE_FS_MOUNT_TYPE_CUSTOM;
	else if (strcmp(mountType, "system") == 0)
		fMountType = PACKAGE_FS_MOUNT_TYPE_SYSTEM;
	else if (strcmp(mountType, "home") == 0)
		fMountType = PACKAGE_FS_MOUNT_TYPE_HOME;
	else if (strcmp(mountType, "custom") == 0)
		fMountType = PACKAGE_FS_MOUNT_TYPE_CUSTOM;
	else
		RETURN_ERROR(B_BAD_VALUE);

	return B_OK;
}


status_t
Volume::_CreateShineThroughDirectory(Directory* parent, const char* name,
	Directory*& _directory)
{
	ShineThroughDirectory* directory = new(std::nothrow) ShineThroughDirectory(
		fNextNodeID++);
	if (directory == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	BReference<ShineThroughDirectory> directoryReference(directory, true);

	String nameString;
	if (!nameString.SetTo(name))
		RETURN_ERROR(B_NO_MEMORY);

	status_t error = directory->Init(parent, nameString);
	if (error != B_OK)
		RETURN_ERROR(error);

	parent->AddChild(directory);

	fNodes.Insert(directory);
	directoryReference.Detach();
		// we keep the initial node reference for the table

	_directory = directory;
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
			case PACKAGE_FS_MOUNT_TYPE_SYSTEM:
			case PACKAGE_FS_MOUNT_TYPE_HOME:
				directories = kShineThroughDirectories;
				break;
			case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
				return B_OK;
			case PACKAGE_FS_MOUNT_TYPE_ENUM_COUNT:
				return B_BAD_VALUE;
		}
	} else if (strcmp(shineThroughSetting, "system") == 0)
		directories = kShineThroughDirectories;
	else if (strcmp(shineThroughSetting, "home") == 0)
		directories = kShineThroughDirectories;
	else if (strcmp(shineThroughSetting, "none") == 0)
		directories = NULL;
	else
		RETURN_ERROR(B_BAD_VALUE);

	if (directories == NULL)
		return B_OK;

	// iterate through the directory list and create the directories
	while (const char* directoryName = *(directories++)) {
		// create the directory
		Directory* directory;
		status_t error = _CreateShineThroughDirectory(fRootDirectory,
			directoryName, directory);
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	return B_OK;
}


status_t
Volume::_PublishShineThroughDirectories()
{
	// Iterate through the root directory children and bind the shine-through
	// directories to the respective mount point subdirectories.
	Node* nextNode;
	for (Node* node = fRootDirectory->FirstChild(); node != NULL;
			node = nextNode) {
		nextNode = fRootDirectory->NextChild(node);

		// skip anything but shine-through directories
		ShineThroughDirectory* directory
			= dynamic_cast<ShineThroughDirectory*>(node);
		if (directory == NULL)
			continue;

		const char* directoryName = directory->Name();

		// look up the mount point subdirectory
		struct vnode* vnode;
		status_t error = vfs_entry_ref_to_vnode(fMountPoint.deviceID,
			fMountPoint.nodeID, directoryName, &vnode);
		if (error != B_OK) {
			dprintf("packagefs: Failed to get shine-through directory \"%s\": "
				"%s\n", directoryName, strerror(error));
			_RemoveNode(directory);
			continue;
		}
		VnodePutter vnodePutter(vnode);

		// stat it
		struct stat st;
		error = vfs_stat_vnode(vnode, &st);
		if (error != B_OK) {
			dprintf("packagefs: Failed to stat shine-through directory \"%s\": "
				"%s\n", directoryName, strerror(error));
			_RemoveNode(directory);
			continue;
		}

		if (!S_ISDIR(st.st_mode)) {
			dprintf("packagefs: Shine-through entry \"%s\" is not a "
				"directory\n", directoryName);
			_RemoveNode(directory);
			continue;
		}

		// publish the vnode, so the VFS will find it without asking us
		directory->AcquireReference();
		error = PublishVNode(directory);
		if (error != B_OK) {
			directory->ReleaseReference();
			_RemoveNode(directory);
			RETURN_ERROR(error);
		}

		// bind the directory
		error = vfs_bind_mount_directory(st.st_dev, st.st_ino, fFSVolume->id,
			directory->ID());

		PutVNode(directory->ID());
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
		fRootDirectory->RemoveChild(packageLinksDirectory);
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


void
Volume::_NotifyNodeAdded(Node* node)
{
	Node* key = node;

	for (int i = 0; i < 2; i++) {
		if (NodeListener* listener = fNodeListeners.Lookup(key)) {
			NodeListener* last = listener->PreviousNodeListener();

			while (true) {
				NodeListener* next = listener->NextNodeListener();

				listener->NodeAdded(node);

				if (listener == last)
					break;

				listener = next;
			}
		}

		key = NULL;
	}
}


void
Volume::_NotifyNodeRemoved(Node* node)
{
	Node* key = node;

	for (int i = 0; i < 2; i++) {
		if (NodeListener* listener = fNodeListeners.Lookup(key)) {
			NodeListener* last = listener->PreviousNodeListener();

			while (true) {
				NodeListener* next = listener->NextNodeListener();

				listener->NodeRemoved(node);

				if (listener == last)
					break;

				listener = next;
			}
		}

		key = NULL;
	}
}


void
Volume::_NotifyNodeChanged(Node* node, uint32 statFields,
	const OldNodeAttributes& oldAttributes)
{
	Node* key = node;

	for (int i = 0; i < 2; i++) {
		if (NodeListener* listener = fNodeListeners.Lookup(key)) {
			NodeListener* last = listener->PreviousNodeListener();

			while (true) {
				NodeListener* next = listener->NextNodeListener();

				listener->NodeChanged(node, statFields, oldAttributes);

				if (listener == last)
					break;

				listener = next;
			}
		}

		key = NULL;
	}
}
