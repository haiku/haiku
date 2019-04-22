/*
 * Copyright 2011-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "packagefs.h"

#include <errno.h>
#include <unistd.h>

#include <package/hpkg/DataReader.h>
#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageDataReader.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageFileHeapReader.h>
#include <package/hpkg/PackageReaderImpl.h>

#include <AutoDeleter.h>
#include <FdIO.h>

#include <util/DoublyLinkedList.h>

#include <Referenceable.h>

#include <boot/PathBlacklist.h>
#include <boot/platform.h>

#include "PackageSettingsItem.h"


#if 0
#	define RETURN_ERROR(error) return (error);
#else
#	define RETURN_ERROR(err)												\
	{																		\
		status_t _status = err;												\
		if (_status < B_OK)													\
			dprintf("%s:%d: %s\n", __FILE__, __LINE__, strerror(_status));	\
		return _status;														\
	}
#endif


using namespace BPackageKit;
using namespace BPackageKit::BHPKG;
using BPackageKit::BHPKG::BPrivate::PackageFileHeapReader;
using BPackageKit::BHPKG::BPrivate::PackageReaderImpl;

using namespace PackageFS;


namespace PackageFS {


struct PackageDirectory;
struct PackageNode;
struct PackageVolume;


static status_t create_node(PackageNode* packageNode, ::Node*& _node);


// #pragma mark - PackageNode


struct PackageNode : DoublyLinkedListLinkImpl<PackageNode> {
	PackageNode(PackageVolume* volume, mode_t mode)
		:
		fVolume(volume),
		fParentDirectory(NULL),
		fName(NULL),
		fNodeID(0),
		fMode(mode)
	{
		fModifiedTime.tv_sec = 0;
		fModifiedTime.tv_nsec = 0;
	}

	virtual ~PackageNode()
	{
		free(fName);
	}

	status_t Init(PackageDirectory* parentDir, const char* name, ino_t nodeID)
	{
		fParentDirectory = parentDir;
		fName = strdup(name);
		fNodeID = nodeID;

		return fName != NULL ? B_OK : B_NO_MEMORY;
	}

	PackageVolume* Volume() const
	{
		return fVolume;
	}

	const char* Name() const
	{
		return fName;
	}

	ino_t NodeID() const
	{
		return fNodeID;
	}

	mode_t Mode() const
	{
		return fMode;
	}

	void SetModifiedTime(const timespec& time)
	{
		fModifiedTime = time;
	}

	const timespec& ModifiedTime() const
	{
		return fModifiedTime;
	}

	virtual void RemoveEntry(const char* path)
	{
	}

protected:
	PackageVolume*		fVolume;
	PackageDirectory*	fParentDirectory;
	char*				fName;
	ino_t				fNodeID;
	mode_t				fMode;
	timespec			fModifiedTime;
};


// #pragma mark - PackageFile


struct PackageFile : PackageNode {
	PackageFile(PackageVolume* volume, mode_t mode, const BPackageData& data)
		:
		PackageNode(volume, mode),
		fData(data)
	{
	}

	const BPackageData& Data() const
	{
		return fData;
	}

	off_t Size() const
	{
		return fData.Size();
	}

private:
	BPackageData	fData;
};


// #pragma mark - PackageSymlink


struct PackageSymlink : PackageNode {
	PackageSymlink(PackageVolume* volume, mode_t mode)
		:
		PackageNode(volume, mode),
		fPath(NULL)
	{
	}

	~PackageSymlink()
	{
		free(fPath);
	}

	status_t SetSymlinkPath(const char* path)
	{
		fPath = strdup(path);
		return fPath != NULL ? B_OK : B_NO_MEMORY;
	}

	const char* SymlinkPath() const
	{
		return fPath;
	}

private:
	char*	fPath;
};


// #pragma mark - PackageDirectory


struct PackageDirectory : PackageNode {
	PackageDirectory(PackageVolume* volume, mode_t mode)
		:
		PackageNode(volume, mode)
	{
	}

	~PackageDirectory()
	{
		while (PackageNode* node = fEntries.RemoveHead())
			delete node;
	}

	void AddChild(PackageNode* node)
	{
		fEntries.Add(node);
	}

	PackageNode* FirstChild() const
	{
		return fEntries.Head();
	}

	PackageNode* NextChild(PackageNode* child) const
	{
		return fEntries.GetNext(child);
	}

	PackageNode* Lookup(const char* name)
	{
		if (strcmp(name, ".") == 0)
			return this;
		if (strcmp(name, "..") == 0)
			return fParentDirectory;

		return _LookupChild(name, strlen(name));
	}

	virtual void RemoveEntry(const char* path)
	{
		const char* componentEnd = strchr(path, '/');
		if (componentEnd == NULL)
			componentEnd = path + strlen(path);

		PackageNode* child = _LookupChild(path, componentEnd - path);
		if (child == NULL)
			return;

		if (*componentEnd == '\0') {
			// last path component -- delete the child
			fEntries.Remove(child);
			delete child;
		} else {
			// must be a directory component -- continue resolving the path
			child->RemoveEntry(componentEnd + 1);
		}
	}

private:
	typedef DoublyLinkedList<PackageNode>	NodeList;

private:
	PackageNode* _LookupChild(const char* name, size_t nameLength)
	{
		for (NodeList::Iterator it = fEntries.GetIterator();
				PackageNode* child = it.Next();) {
			if (strncmp(child->Name(), name, nameLength) == 0
				&& child->Name()[nameLength] == '\0') {
				return child;
			}
		}

		return NULL;
	}

private:
	NodeList	fEntries;
};


// #pragma mark - PackageLoaderErrorOutput


struct PackageLoaderErrorOutput : BErrorOutput {
	PackageLoaderErrorOutput()
	{
	}

	virtual void PrintErrorVarArgs(const char* format, va_list args)
	{
		char buffer[256];
		vsnprintf(buffer, sizeof(buffer), format, args);
		dprintf("%s", buffer);
	}
};


// #pragma mark - PackageVolume


struct PackageVolume : BReferenceable, private PackageLoaderErrorOutput {
	PackageVolume()
		:
		fNextNodeID(1),
		fRootDirectory(this, S_IFDIR),
		fHeapReader(NULL),
		fFile(NULL)
	{
	}

	~PackageVolume()
	{
		delete fHeapReader;
		delete fFile;
	}

	status_t Init(int fd, const PackageFileHeapReader* heapReader)
	{
		status_t error = fRootDirectory.Init(&fRootDirectory, ".",
			NextNodeID());
		if (error != B_OK)
			return error;

		fd = dup(fd);
		if (fd < 0)
			return errno;

		fFile = new(std::nothrow) BFdIO(fd, true);
		if (fFile == NULL) {
			close(fd);
			return B_NO_MEMORY;
		}

		// clone a heap reader and adjust it for our use
		fHeapReader = heapReader->Clone();
		if (fHeapReader == NULL)
			return B_NO_MEMORY;

		fHeapReader->SetErrorOutput(this);
		fHeapReader->SetFile(fFile);

		return B_OK;
	}

	PackageDirectory* RootDirectory()
	{
		return &fRootDirectory;
	}

	ino_t NextNodeID()
	{
		return fNextNodeID++;
	}

	status_t CreateFileDataReader(const BPackageData& data,
		BAbstractBufferedDataReader*& _reader)
	{
		return BPackageDataReaderFactory().CreatePackageDataReader(fHeapReader,
			data, _reader);
	}

private:
	ino_t						fNextNodeID;
	PackageDirectory			fRootDirectory;
	PackageFileHeapReader*		fHeapReader;
	BPositionIO*				fFile;
};


// #pragma mark - PackageLoaderContentHandler


struct PackageLoaderContentHandler : BPackageContentHandler {
	PackageLoaderContentHandler(PackageVolume* volume,
		PackageSettingsItem* settingsItem)
		:
		fVolume(volume),
		fSettingsItem(settingsItem),
		fLastSettingsEntry(NULL),
		fLastSettingsEntryEntry(NULL),
		fErrorOccurred(false)
	{
	}

	status_t Init()
	{
		return B_OK;
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		if (fErrorOccurred
			|| (fLastSettingsEntry != NULL
				&& fLastSettingsEntry->IsBlackListed())) {
			return B_OK;
		}

		PackageDirectory* parentDir = NULL;
		if (const BPackageEntry* parentEntry = entry->Parent()) {
			if (!S_ISDIR(parentEntry->Mode()))
				RETURN_ERROR(B_BAD_DATA);

			parentDir = static_cast<PackageDirectory*>(
				(PackageNode*)parentEntry->UserToken());
		}

		if (fSettingsItem != NULL
			&& (parentDir == NULL
				|| entry->Parent() == fLastSettingsEntryEntry)) {
			PackageSettingsItem::Entry* settingsEntry
				= fSettingsItem->FindEntry(fLastSettingsEntry, entry->Name());
			if (settingsEntry != NULL) {
				fLastSettingsEntry = settingsEntry;
				fLastSettingsEntryEntry = entry;
				if (fLastSettingsEntry->IsBlackListed())
					return B_OK;
			}
		}

		if (parentDir == NULL)
			parentDir = fVolume->RootDirectory();

		status_t error;

		// get the file mode -- filter out write permissions
		mode_t mode = entry->Mode() & ~(mode_t)(S_IWUSR | S_IWGRP | S_IWOTH);

		// create the package node
		PackageNode* node;
		if (S_ISREG(mode)) {
			// file
			node = new(std::nothrow) PackageFile(fVolume, mode, entry->Data());
		} else if (S_ISLNK(mode)) {
			// symlink
			PackageSymlink* symlink = new(std::nothrow) PackageSymlink(
				fVolume, mode);
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
			node = new(std::nothrow) PackageDirectory(fVolume, mode);
		} else
			RETURN_ERROR(B_BAD_DATA);

		if (node == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		error = node->Init(parentDir, entry->Name(), fVolume->NextNodeID());
		if (error != B_OK) {
			delete node;
			RETURN_ERROR(error);
		}

		node->SetModifiedTime(entry->ModifiedTime());

		// add it to the parent directory
		parentDir->AddChild(node);

		entry->SetUserToken(node);

		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		// attributes aren't needed in the boot loader
		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		if (entry == fLastSettingsEntryEntry) {
			fLastSettingsEntryEntry = entry->Parent();
			fLastSettingsEntry = fLastSettingsEntry->Parent();
		}

		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		// TODO?
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
		fErrorOccurred = true;
	}

private:
	PackageVolume*				fVolume;
	const PackageSettingsItem*	fSettingsItem;
	PackageSettingsItem::Entry*	fLastSettingsEntry;
	const BPackageEntry*		fLastSettingsEntryEntry;
	bool						fErrorOccurred;
};


// #pragma mark - File


struct File : ::Node {
	File(PackageFile* file)
		:
		fFile(file)
	{
		fFile->Volume()->AcquireReference();
	}

	~File()
	{
		fFile->Volume()->ReleaseReference();
	}

	virtual ssize_t ReadAt(void* cookie, off_t pos, void* buffer,
		size_t bufferSize)
	{
		off_t size = fFile->Size();
		if (pos < 0 || pos > size)
			return B_BAD_VALUE;
		if (pos + (off_t)bufferSize > size)
			bufferSize = size - pos;

		if (bufferSize > 0) {
			BAbstractBufferedDataReader* dataReader
				= (BAbstractBufferedDataReader*)cookie;
			status_t error = dataReader->ReadData(pos, buffer, bufferSize);
			if (error != B_OK)
				return error;
		}

		return bufferSize;
	}

	virtual ssize_t WriteAt(void* cookie, off_t pos, const void *buffer,
		size_t bufferSize)
	{
		return B_READ_ONLY_DEVICE;
	}

	virtual status_t GetName(char* nameBuffer, size_t bufferSize) const
	{
		strlcpy(nameBuffer, fFile->Name(), bufferSize);
		return B_OK;
	}

	virtual status_t Open(void** _cookie, int mode)
	{
		if ((mode & O_ACCMODE) != O_RDONLY && (mode & O_ACCMODE) != O_RDWR)
			return B_NOT_ALLOWED;

		BAbstractBufferedDataReader* dataReader;
		status_t error = fFile->Volume()->CreateFileDataReader(fFile->Data(),
			dataReader);
		if (error != B_OK)
			return error;

		*_cookie = dataReader;
		Acquire();
		return B_OK;
	}

	virtual status_t Close(void* cookie)
	{
		BAbstractBufferedDataReader* dataReader
			= (BAbstractBufferedDataReader*)cookie;
		delete dataReader;
		Release();
		return B_OK;
	}


	virtual int32 Type() const
	{
		return fFile->Mode() & S_IFMT;
	}

	virtual off_t Size() const
	{
		return fFile->Size();
	}

	virtual ino_t Inode() const
	{
		return fFile->NodeID();
	}

private:
	PackageFile*	fFile;
};


// #pragma mark - Symlink


struct Symlink : ::Node {
	Symlink(PackageSymlink* symlink)
		:
		fSymlink(symlink)
	{
		fSymlink->Volume()->AcquireReference();
	}

	~Symlink()
	{
		fSymlink->Volume()->ReleaseReference();
	}

	virtual ssize_t ReadAt(void* cookie, off_t pos, void* buffer,
		size_t bufferSize)
	{
		return B_BAD_VALUE;
	}

	virtual ssize_t WriteAt(void* cookie, off_t pos, const void *buffer,
		size_t bufferSize)
	{
		return B_READ_ONLY_DEVICE;
	}

	virtual status_t ReadLink(char* buffer, size_t bufferSize)
	{
		const char* path = fSymlink->SymlinkPath();
		size_t size = strlen(path) + 1;

		if (size > bufferSize)
			return B_BUFFER_OVERFLOW;

		memcpy(buffer, path, size);
		return B_OK;
	}

	virtual status_t GetName(char* nameBuffer, size_t bufferSize) const
	{
		strlcpy(nameBuffer, fSymlink->Name(), bufferSize);
		return B_OK;
	}

	virtual int32 Type() const
	{
		return fSymlink->Mode() & S_IFMT;
	}

	virtual off_t Size() const
	{
		return strlen(fSymlink->SymlinkPath()) + 1;
	}

	virtual ino_t Inode() const
	{
		return fSymlink->NodeID();
	}

private:
	PackageSymlink*	fSymlink;
};


// #pragma mark - Directory


struct Directory : ::Directory {
	Directory(PackageDirectory* symlink)
		:
		fDirectory(symlink)
	{
		fDirectory->Volume()->AcquireReference();
	}

	~Directory()
	{
		fDirectory->Volume()->ReleaseReference();
	}

	void RemoveEntry(const char* path)
	{
		fDirectory->RemoveEntry(path);
	}

	virtual ssize_t ReadAt(void* cookie, off_t pos, void* buffer,
		size_t bufferSize)
	{
		return B_IS_A_DIRECTORY;
	}

	virtual ssize_t WriteAt(void* cookie, off_t pos, const void *buffer,
		size_t bufferSize)
	{
		return B_IS_A_DIRECTORY;
	}

	virtual status_t GetName(char* nameBuffer, size_t bufferSize) const
	{
		strlcpy(nameBuffer, fDirectory->Name(), bufferSize);
		return B_OK;
	}

	virtual int32 Type() const
	{
		return fDirectory->Mode() & S_IFMT;
	}

	virtual ino_t Inode() const
	{
		return fDirectory->NodeID();
	}

	virtual status_t Open(void** _cookie, int mode)
	{
		if ((mode & O_ACCMODE) != O_RDONLY && (mode & O_ACCMODE) != O_RDWR)
			return B_NOT_ALLOWED;

		Cookie* cookie = new(std::nothrow) Cookie;
		if (cookie == NULL)
			return B_NO_MEMORY;

		cookie->nextChild = fDirectory->FirstChild();

		Acquire();
		*_cookie = cookie;
		return B_OK;
	}

	virtual status_t Close(void* _cookie)
	{
		Cookie* cookie = (Cookie*)_cookie;
		delete cookie;
		Release();
		return B_OK;
	}

	virtual Node* LookupDontTraverse(const char* name)
	{
		// look up the child
		PackageNode* child = fDirectory->Lookup(name);
		if (child == NULL)
			return NULL;

		// create the node
		::Node* node;
		return create_node(child, node) == B_OK ? node : NULL;
	}

	virtual status_t GetNextEntry(void* _cookie, char* nameBuffer,
		size_t bufferSize)
	{
		Cookie* cookie = (Cookie*)_cookie;
		PackageNode* child = cookie->nextChild;
		if (child == NULL)
			return B_ENTRY_NOT_FOUND;

		cookie->nextChild = fDirectory->NextChild(child);

		strlcpy(nameBuffer, child->Name(), bufferSize);
		return B_OK;
	}

	virtual status_t GetNextNode(void* _cookie, Node** _node)
	{
		Cookie* cookie = (Cookie*)_cookie;
		PackageNode* child = cookie->nextChild;
		if (child == NULL)
			return B_ENTRY_NOT_FOUND;

		cookie->nextChild = fDirectory->NextChild(child);

		return create_node(child, *_node);
	}

	virtual status_t Rewind(void* _cookie)
	{
		Cookie* cookie = (Cookie*)_cookie;
		cookie->nextChild = NULL;
		return B_OK;
	}

	virtual bool IsEmpty()
	{
		return fDirectory->FirstChild() == NULL;
	}

private:
	struct Cookie {
		PackageNode*	nextChild;
	};


private:
	PackageDirectory*	fDirectory;
};


// #pragma mark -


static status_t
create_node(PackageNode* packageNode, ::Node*& _node)
{
	if (packageNode == NULL)
		return B_BAD_VALUE;

	::Node* node;
	switch (packageNode->Mode() & S_IFMT) {
		case S_IFREG:
			node = new(std::nothrow) File(
				static_cast<PackageFile*>(packageNode));
			break;
		case S_IFLNK:
			node = new(std::nothrow) Symlink(
				static_cast<PackageSymlink*>(packageNode));
			break;
		case S_IFDIR:
			node = new(std::nothrow) Directory(
				static_cast<PackageDirectory*>(packageNode));
			break;
		default:
			return B_BAD_VALUE;
	}

	if (node == NULL)
		return B_NO_MEMORY;

	_node = node;
	return B_OK;
}


}	// namespace PackageFS


status_t
packagefs_mount_file(int fd, ::Directory* systemDirectory,
	::Directory*& _mountedDirectory)
{
	PackageLoaderErrorOutput errorOutput;
 	PackageReaderImpl packageReader(&errorOutput);
	status_t error = packageReader.Init(fd, false, 0);
 	if (error != B_OK)
 		RETURN_ERROR(error);

	// create the volume
	PackageVolume* volume = new(std::nothrow) PackageVolume;
	if (volume == NULL)
		return B_NO_MEMORY;
	BReference<PackageVolume> volumeReference(volume, true);

	error = volume->Init(fd, packageReader.RawHeapReader());
	if (error != B_OK)
		RETURN_ERROR(error);

	// load settings for the package
	PackageSettingsItem* settings = PackageSettingsItem::Load(systemDirectory,
		"haiku");
	ObjectDeleter<PackageSettingsItem> settingsDeleter(settings);

	// parse content -- this constructs the entry/node tree
	PackageLoaderContentHandler handler(volume, settings);
	error = handler.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = packageReader.ParseContent(&handler);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create a VFS node for the root node
	::Node* rootNode;
	error = create_node(volume->RootDirectory(), rootNode);
	if (error != B_OK)
		RETURN_ERROR(error);

	_mountedDirectory = static_cast< ::Directory*>(rootNode);
	return B_OK;
}


void
packagefs_apply_path_blacklist(::Directory* systemDirectory,
	const PathBlacklist& pathBlacklist)
{
	PackageFS::Directory* directory
		= static_cast<PackageFS::Directory*>(systemDirectory);

	for (PathBlacklist::Iterator it = pathBlacklist.GetIterator();
		BlacklistedPath* path = it.Next();) {
		directory->RemoveEntry(path->Path());
	}
}

