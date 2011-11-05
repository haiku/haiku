/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Volume.h"

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include "FileSystem.h"
#include "kernel_emu.h"


// constructor
Volume::Volume(FileSystem* fileSystem, dev_t id)
	: fFileSystem(fileSystem),
	  fID(id)
{
	fFileSystem->RegisterVolume(this);
}

// destructor
Volume::~Volume()
{
	fFileSystem->UnregisterVolume(this);
}

// GetFileSystem
UserlandFS::FileSystem*
Volume::GetFileSystem() const
{
	return fFileSystem;
}

// GetID
dev_t
Volume::GetID() const
{
	return fID;
}


// #pragma mark - FS


// Mount
status_t
Volume::Mount(const char* device, uint32 flags, const char* parameters,
	ino_t* rootID)
{
	return B_BAD_VALUE;
}

// Unmount
status_t
Volume::Unmount()
{
	return B_BAD_VALUE;
}

// Sync
status_t
Volume::Sync()
{
	return B_BAD_VALUE;
}

// ReadFSInfo
status_t
Volume::ReadFSInfo(fs_info* info)
{
	return B_BAD_VALUE;
}

// WriteFSInfo
status_t
Volume::WriteFSInfo(const struct fs_info* info, uint32 mask)
{
	return B_BAD_VALUE;
}


// #pragma mark - vnodes


// Lookup
status_t
Volume::Lookup(void* dir, const char* entryName, ino_t* vnid)
{
	return B_BAD_VALUE;
}


// GetVNodeType
status_t
Volume::GetVNodeType(void* node, int* type)
{
	return B_UNSUPPORTED;
}


// GetVNodeName
status_t
Volume::GetVNodeName(void* node, char* buffer, size_t bufferSize)
{
	// stat the node to get its ID
	struct stat st;
	status_t error = ReadStat(node, &st);
	if (error != B_OK)
		return error;

	// look up the parent directory
	ino_t parentID;
	error = Lookup(node, "..", &parentID);
	if (error != B_OK)
		return error;

	// get the parent node handle
	void* parentNode;
	error = UserlandFS::KernelEmu::get_vnode(GetID(), parentID, &parentNode);
	// Lookup() has already called get_vnode() for us, so we need to put it once
	UserlandFS::KernelEmu::put_vnode(GetID(), parentID);
	if (error != B_OK)
		return error;

	// open the parent dir
	void* cookie;
	error = OpenDir(parentNode, &cookie);
	if (error == B_OK) {

		while (true) {
			// read an entry
			char _entry[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
			struct dirent* entry = (struct dirent*)_entry;
			uint32 num;

			error = ReadDir(parentNode, cookie, entry, sizeof(_entry), 1, &num);

			if (error != B_OK)
				break;
			if (num == 0) {
				error = B_ENTRY_NOT_FOUND;
				break;
			}

			// found an entry for our node?
			if (st.st_ino == entry->d_ino) {
				// yep, copy the entry name
				size_t nameLen = strnlen(entry->d_name, B_FILE_NAME_LENGTH);
				if (nameLen < bufferSize) {
					memcpy(buffer, entry->d_name, nameLen);
					buffer[nameLen] = '\0';
				} else
					error = B_BUFFER_OVERFLOW;
				break;
			}
		}

		// close the parent dir
		CloseDir(parentNode, cookie);
		FreeDirCookie(parentNode, cookie);
	}

	// put the parent node
	UserlandFS::KernelEmu::put_vnode(GetID(), parentID);

	return error;
}

// ReadVNode
status_t
Volume::ReadVNode(ino_t vnid, bool reenter, void** node, int* type,
	uint32* flags, FSVNodeCapabilities* _capabilities)
{
	return B_BAD_VALUE;
}

// WriteVNode
status_t
Volume::WriteVNode(void* node, bool reenter)
{
	return B_BAD_VALUE;
}

// RemoveVNode
status_t
Volume::RemoveVNode(void* node, bool reenter)
{
	return B_BAD_VALUE;
}


// #pragma mark - asynchronous I/O


status_t
Volume::DoIO(void* node, void* cookie, const IORequestInfo& requestInfo)
{
	return B_BAD_VALUE;
}


status_t
Volume::CancelIO(void* node, void* cookie, int32 ioRequestID)
{
	return B_BAD_VALUE;
}


status_t
Volume::IterativeIOGetVecs(void* cookie, int32 requestID, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	return B_BAD_VALUE;
}


status_t
Volume::IterativeIOFinished(void* cookie, int32 requestID, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	return B_BAD_VALUE;
}


// #pragma mark - nodes


// IOCtl
status_t
Volume::IOCtl(void* node, void* cookie, uint32 command, void *buffer,
	size_t size)
{
	return B_BAD_VALUE;
}

// SetFlags
status_t
Volume::SetFlags(void* node, void* cookie, int flags)
{
	return B_BAD_VALUE;
}

// Select
status_t
Volume::Select(void* node, void* cookie, uint8 event, selectsync* sync)
{
	return B_BAD_VALUE;
}

// Deselect
status_t
Volume::Deselect(void* node, void* cookie, uint8 event, selectsync* sync)
{
	return B_BAD_VALUE;
}

// FSync
status_t
Volume::FSync(void* node)
{
	return B_BAD_VALUE;
}

// ReadSymlink
status_t
Volume::ReadSymlink(void* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	return B_BAD_VALUE;
}

// CreateSymlink
status_t
Volume::CreateSymlink(void* dir, const char* name, const char* target,
	int mode)
{
	return B_BAD_VALUE;
}

// Link
status_t
Volume::Link(void* dir, const char* name, void* node)
{
	return B_BAD_VALUE;
}

// Unlink
status_t
Volume::Unlink(void* dir, const char* name)
{
	return B_BAD_VALUE;
}

// Rename
status_t
Volume::Rename(void* oldDir, const char* oldName, void* newDir,
	const char* newName)
{
	return B_BAD_VALUE;
}

// Access
status_t
Volume::Access(void* node, int mode)
{
	return B_BAD_VALUE;
}

// ReadStat
status_t
Volume::ReadStat(void* node, struct stat* st)
{
	return B_BAD_VALUE;
}

// WriteStat
status_t
Volume::WriteStat(void* node, const struct stat *st, uint32 mask)
{
	return B_BAD_VALUE;
}


// #pragma mark - files


// Create
status_t
Volume::Create(void* dir, const char* name, int openMode, int mode,
	void** cookie, ino_t* vnid)
{
	return B_BAD_VALUE;
}

// Open
status_t
Volume::Open(void* node, int openMode, void** cookie)
{
	return B_BAD_VALUE;
}

// Close
status_t
Volume::Close(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeCookie
status_t
Volume::FreeCookie(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// Read
status_t
Volume::Read(void* node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	return B_BAD_VALUE;
}

// Write
status_t
Volume::Write(void* node, void* cookie, off_t pos, const void* buffer,
	size_t bufferSize, size_t* bytesWritten)
{
	return B_BAD_VALUE;
}


// #pragma mark - directories


// CreateDir
status_t
Volume::CreateDir(void* dir, const char* name, int mode)
{
	return B_BAD_VALUE;
}

// RemoveDir
status_t
Volume::RemoveDir(void* dir, const char* name)
{
	return B_BAD_VALUE;
}

// OpenDir
status_t
Volume::OpenDir(void* node, void** cookie)
{
	return B_BAD_VALUE;
}

// CloseDir
status_t
Volume::CloseDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeDirCookie
status_t
Volume::FreeDirCookie(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// ReadDir
status_t
Volume::ReadDir(void* node, void* cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	return B_BAD_VALUE;
}

// RewindDir
status_t
Volume::RewindDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}


// #pragma mark - attribute directories


// OpenAttrDir
status_t
Volume::OpenAttrDir(void* node, void** cookie)
{
	return B_BAD_VALUE;
}

// CloseAttrDir
status_t
Volume::CloseAttrDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeAttrDirCookie
status_t
Volume::FreeAttrDirCookie(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// ReadAttrDir
status_t
Volume::ReadAttrDir(void* node, void* cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	return B_BAD_VALUE;
}

// RewindAttrDir
status_t
Volume::RewindAttrDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}


// #pragma mark - attributes


// CreateAttr
status_t
Volume::CreateAttr(void* node, const char* name, uint32 type, int openMode,
	void** cookie)
{
	return B_BAD_VALUE;
}

// OpenAttr
status_t
Volume::OpenAttr(void* node, const char* name, int openMode,
	void** cookie)
{
	return B_BAD_VALUE;
}

// CloseAttr
status_t
Volume::CloseAttr(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeAttrCookie
status_t
Volume::FreeAttrCookie(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// ReadAttr
status_t
Volume::ReadAttr(void* node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	return B_BAD_VALUE;
}

// WriteAttr
status_t
Volume::WriteAttr(void* node, void* cookie, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	return B_BAD_VALUE;
}

// ReadAttrStat
status_t
Volume::ReadAttrStat(void* node, void* cookie, struct stat *st)
{
	return B_BAD_VALUE;
}

// WriteAttrStat
status_t
Volume::WriteAttrStat(void* node, void* cookie, const struct stat* st,
	int statMask)
{
	return B_BAD_VALUE;
}

// RenameAttr
status_t
Volume::RenameAttr(void* oldNode, const char* oldName, void* newNode,
	const char* newName)
{
	return B_BAD_VALUE;
}

// RemoveAttr
status_t
Volume::RemoveAttr(void* node, const char* name)
{
	return B_BAD_VALUE;
}


// #pragma mark - indices


// OpenIndexDir
status_t
Volume::OpenIndexDir(void** cookie)
{
	return B_BAD_VALUE;
}

// CloseIndexDir
status_t
Volume::CloseIndexDir(void* cookie)
{
	return B_BAD_VALUE;
}

// FreeIndexDirCookie
status_t
Volume::FreeIndexDirCookie(void* cookie)
{
	return B_BAD_VALUE;
}

// ReadIndexDir
status_t
Volume::ReadIndexDir(void* cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	return B_BAD_VALUE;
}

// RewindIndexDir
status_t
Volume::RewindIndexDir(void* cookie)
{
	return B_BAD_VALUE;
}

// CreateIndex
status_t
Volume::CreateIndex(const char* name, uint32 type, uint32 flags)
{
	return B_BAD_VALUE;
}

// RemoveIndex
status_t
Volume::RemoveIndex(const char* name)
{
	return B_BAD_VALUE;
}

// ReadIndexStat
status_t
Volume::ReadIndexStat(const char *name, struct stat *st)
{
	return B_BAD_VALUE;
}


// #pragma mark - queries


// OpenQuery
status_t
Volume::OpenQuery(const char* queryString, uint32 flags, port_id port,
	uint32 token, void** cookie)
{
	return B_BAD_VALUE;
}

// CloseQuery
status_t
Volume::CloseQuery(void* cookie)
{
	return B_BAD_VALUE;
}

// FreeQueryCookie
status_t
Volume::FreeQueryCookie(void* cookie)
{
	return B_BAD_VALUE;
}

// ReadQuery
status_t
Volume::ReadQuery(void* cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	return B_BAD_VALUE;
}

// RewindQuery
status_t
Volume::RewindQuery(void* cookie)
{
	return B_BAD_VALUE;
}

