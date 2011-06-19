/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "packagefs.h"

#include <errno.h>
#include <unistd.h>

#include <package/BlockBufferCacheNoLock.h>
#include <package/hpkg/DataReader.h>
#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageDataReader.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReaderImpl.h>

#include <AutoDeleter.h>

#include <util/DoublyLinkedList.h>

#include <Referenceable.h>

#include <boot/platform.h>


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
using BPackageKit::BHPKG::BPrivate::PackageReaderImpl;


namespace {


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
		fMode(mode)
	{
		fModifiedTime.tv_sec = 0;
		fModifiedTime.tv_nsec = 0;
	}

	~PackageNode()
	{
		free(fName);
	}

	status_t Init(PackageDirectory* parentDir, const char* name)
	{
		fParentDirectory = parentDir;
		fName = strdup(name);

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

protected:
	PackageVolume*		fVolume;
	PackageDirectory*	fParentDirectory;
	char*				fName;
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
		return fData.UncompressedSize();
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

	PackageNode* Lookup(const char* name) const
	{
		for (NodeList::ConstIterator it = fEntries.GetIterator();
				PackageNode* child = it.Next();) {
			if (strcmp(child->Name(), name) == 0)
				return child;
		}

		return NULL;
	}

private:
	typedef DoublyLinkedList<PackageNode>	NodeList;

private:
	NodeList	fEntries;
};


// #pragma mark - FileDataReader


struct FileDataReader {
	FileDataReader(int fd, const BPackageData& data)
		:
		fDataReader(fd),
		fData(data),
		fPackageDataReader(NULL)
	{
	}

	~FileDataReader()
	{
		delete fPackageDataReader;
	}

	status_t Init(BPackageDataReaderFactory* dataReaderFactory)
	{
		if (fData.IsEncodedInline())
			return B_OK;

		return dataReaderFactory->CreatePackageDataReader(&fDataReader,
			fData, fPackageDataReader);
	}

	status_t ReadData(off_t offset, void* buffer, size_t bufferSize)
	{
		if (fData.IsEncodedInline()) {
			BBufferDataReader dataReader(fData.InlineData(),
				fData.CompressedSize());
			return dataReader.ReadData(offset, buffer, bufferSize);
		}

		return fPackageDataReader->ReadData(offset, buffer, bufferSize);
	}

private:
	BFDDataReader		fDataReader;
	BPackageData		fData;
	BPackageDataReader*	fPackageDataReader;
};


// #pragma mark - PackageVolume


struct PackageVolume : BReferenceable {
	PackageVolume()
		:
		fRootDirectory(this, S_IFDIR),
		fBufferCache(B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB, 2),
		fDataReaderFactory(&fBufferCache),
		fFD(-1)
	{
	}

	~PackageVolume()
	{
		if (fFD >= 0)
			close(fFD);
	}

	status_t Init(int fd)
	{
		status_t error = fBufferCache.Init();
		if (error != B_OK)
			return error;

		fFD = dup(fd);
		if (fFD < 0)
			return errno;

		return B_OK;
	}

	PackageDirectory* RootDirectory()
	{
		return &fRootDirectory;
	}

	void AddNode(PackageNode* node)
	{
		fRootDirectory.AddChild(node);
	}

	status_t CreateFileDataReader(const BPackageData& data,
		FileDataReader*& _fileDataReader)
	{
		// create the file reader object
		FileDataReader* fileDataReader = new(std::nothrow) FileDataReader(fFD,
			data);
		if (fileDataReader == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<FileDataReader> fileDataReaderDeleter(fileDataReader);

		status_t error = fileDataReader->Init(&fDataReaderFactory);
		if (error != B_OK)
			return error;

		_fileDataReader = fileDataReaderDeleter.Detach();
		return B_OK;
	}

private:
	PackageDirectory			fRootDirectory;
	BBlockBufferCacheNoLock		fBufferCache;
	BPackageDataReaderFactory	fDataReaderFactory;
	int							fFD;
};


// #pragma mark - PackageLoaderErrorOutput


struct PackageLoaderErrorOutput : BErrorOutput {
	PackageLoaderErrorOutput()
	{
	}

	virtual void PrintErrorVarArgs(const char* format, va_list args)
	{
	}
};


// #pragma mark - PackageLoaderContentHandler


struct PackageLoaderContentHandler : BPackageContentHandler {
	PackageLoaderContentHandler(PackageVolume* volume)
		:
		fVolume(volume),
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
		if (const BPackageEntry* parentEntry = entry->Parent()) {
			if (!S_ISDIR(parentEntry->Mode()))
				RETURN_ERROR(B_BAD_DATA);

			parentDir = static_cast<PackageDirectory*>(
				(PackageNode*)parentEntry->UserToken());
		}

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

		error = node->Init(parentDir, entry->Name());
		if (error != B_OK) {
			delete node;
			RETURN_ERROR(error);
		}

		node->SetModifiedTime(entry->ModifiedTime());

		// add it to the parent directory
		if (parentDir != NULL)
			parentDir->AddChild(node);
		else
			fVolume->AddNode(node);

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
	PackageVolume*	fVolume;
	bool			fErrorOccurred;
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
		if (pos + bufferSize > size)
			bufferSize = size - pos;

		if (bufferSize > 0) {
			FileDataReader* dataReader = (FileDataReader*)cookie;
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

		FileDataReader* dataReader;
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
		FileDataReader* dataReader = (FileDataReader*)cookie;
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


}	// unnamed namespace


status_t
packagefs_mount_file(int fd, ::Directory*& _mountedDirectory)
{
	PackageLoaderErrorOutput errorOutput;
 	PackageReaderImpl packageReader(&errorOutput);
 	status_t error = packageReader.Init(fd, false);
 	if (error != B_OK)
 		RETURN_ERROR(error);

	// create the volume
	PackageVolume* volume = new(std::nothrow) PackageVolume;
	if (volume == NULL)
		return B_NO_MEMORY;
	BReference<PackageVolume> volumeReference(volume, true);

	error = volume->Init(fd);
	if (error != B_OK)
		RETURN_ERROR(error);

	// parse content -- this constructs the entry/node tree
	PackageLoaderContentHandler handler(volume);
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
