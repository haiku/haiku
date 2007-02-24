// UserVolume.cpp

#include "UserVolume.h"

// constructor
UserVolume::UserVolume(UserFileSystem* fileSystem, nspace_id id)
	: fFileSystem(fileSystem),
	  fID(id)
{
}

// destructor
UserVolume::~UserVolume()
{
}

// GetFileSystem
UserlandFS::UserFileSystem*
UserVolume::GetFileSystem() const
{
	return fFileSystem;
}

// GetID
nspace_id
UserVolume::GetID() const
{
	return fID;
}

// #pragma mark -
// #pragma mark ----- FS -----

// Mount
status_t
UserVolume::Mount(const char* device, ulong flags, const char* parameters,
	int32 len, vnode_id* rootID)
{
	return B_BAD_VALUE;
}

// Unmount
status_t
UserVolume::Unmount()
{
	return B_BAD_VALUE;
}

// Sync
status_t
UserVolume::Sync()
{
	return B_BAD_VALUE;
}

// ReadFSStat
status_t
UserVolume::ReadFSStat(fs_info* info)
{
	return B_BAD_VALUE;
}

// WriteFSStat
status_t
UserVolume::WriteFSStat(struct fs_info *info, long mask)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- vnodes -----

// ReadVNode
status_t
UserVolume::ReadVNode(vnode_id vnid, char reenter, void** node)
{
	return B_BAD_VALUE;
}

// WriteVNode
status_t
UserVolume::WriteVNode(void* node, char reenter)
{
	return B_BAD_VALUE;
}

// RemoveVNode
status_t
UserVolume::RemoveVNode(void* node, char reenter)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- nodes -----

// FSync
status_t
UserVolume::FSync(void* node)
{
	return B_BAD_VALUE;
}

// ReadStat
status_t
UserVolume::ReadStat(void* node, struct stat* st)
{
	return B_BAD_VALUE;
}

// WriteStat
status_t
UserVolume::WriteStat(void* node, struct stat* st, long mask)
{
	return B_BAD_VALUE;
}

// Access
status_t
UserVolume::Access(void* node, int mode)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- files -----

// Create
status_t
UserVolume::Create(void* dir, const char* name, int openMode, int mode,
	vnode_id* vnid, void** cookie)
{
	return B_BAD_VALUE;
}

// Open
status_t
UserVolume::Open(void* node, int openMode, void** cookie)
{
	return B_BAD_VALUE;
}

// Close
status_t
UserVolume::Close(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeCookie
status_t
UserVolume::FreeCookie(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// Read
status_t
UserVolume::Read(void* node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	return B_BAD_VALUE;
}

// Write
status_t
UserVolume::Write(void* node, void* cookie, off_t pos, const void* buffer,
	size_t bufferSize, size_t* bytesWritten)
{
	return B_BAD_VALUE;
}

// IOCtl
status_t
UserVolume::IOCtl(void* node, void* cookie, int command, void *buffer,
	size_t size)
{
	return B_BAD_VALUE;
}

// SetFlags
status_t
UserVolume::SetFlags(void* node, void* cookie, int flags)
{
	return B_BAD_VALUE;
}

// Select
status_t
UserVolume::Select(void* node, void* cookie, uint8 event, uint32 ref,
	selectsync* sync)
{
	return B_BAD_VALUE;
}

// Deselect
status_t
UserVolume::Deselect(void* node, void* cookie, uint8 event, selectsync* sync)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// Link
status_t
UserVolume::Link(void* dir, const char* name, void* node)
{
	return B_BAD_VALUE;
}

// Unlink
status_t
UserVolume::Unlink(void* dir, const char* name)
{
	return B_BAD_VALUE;
}

// Symlink
status_t
UserVolume::Symlink(void* dir, const char* name, const char* target)
{
	return B_BAD_VALUE;
}

// ReadLink
status_t
UserVolume::ReadLink(void* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	return B_BAD_VALUE;
}

// Rename
status_t
UserVolume::Rename(void* oldDir, const char* oldName, void* newDir,
	const char* newName)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- directories -----

// MkDir
status_t
UserVolume::MkDir(void* dir, const char* name, int mode)
{
	return B_BAD_VALUE;
}

// RmDir
status_t
UserVolume::RmDir(void* dir, const char* name)
{
	return B_BAD_VALUE;
}

// OpenDir
status_t
UserVolume::OpenDir(void* node, void** cookie)
{
	return B_BAD_VALUE;
}

// CloseDir
status_t
UserVolume::CloseDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeDirCookie
status_t
UserVolume::FreeDirCookie(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// ReadDir
status_t
UserVolume::ReadDir(void* node, void* cookie, void* buffer, size_t bufferSize,
	int32 count, int32* countRead)
{
	return B_BAD_VALUE;
}

// RewindDir
status_t
UserVolume::RewindDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// Walk
status_t
UserVolume::Walk(void* dir, const char* entryName, char** resolvedPath,
	vnode_id* vnid)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- attributes -----

// OpenAttrDir
status_t
UserVolume::OpenAttrDir(void* node, void** cookie)
{
	return B_BAD_VALUE;
}

// CloseAttrDir
status_t
UserVolume::CloseAttrDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeAttrDirCookie
status_t
UserVolume::FreeAttrDirCookie(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// ReadAttrDir
status_t
UserVolume::ReadAttrDir(void* node, void* cookie, void* buffer, size_t bufferSize,
	int32 count, int32* countRead)
{
	return B_BAD_VALUE;
}

// RewindAttrDir
status_t
UserVolume::RewindAttrDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// ReadAttr
status_t
UserVolume::ReadAttr(void* node, const char* name, int type, off_t pos,
	void* buffer, size_t bufferSize, size_t* bytesRead)
{
	return B_BAD_VALUE;
}

// WriteAttr
status_t
UserVolume::WriteAttr(void* node, const char* name, int type, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	return B_BAD_VALUE;
}

// RemoveAttr
status_t
UserVolume::RemoveAttr(void* node, const char* name)
{
	return B_BAD_VALUE;
}

// RenameAttr
status_t
UserVolume::RenameAttr(void* node, const char* oldName, const char* newName)
{
	return B_BAD_VALUE;
}

// StatAttr
status_t
UserVolume::StatAttr(void* node, const char* name, struct attr_info* attrInfo)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- indices -----

// OpenIndexDir
status_t
UserVolume::OpenIndexDir(void** cookie)
{
	return B_BAD_VALUE;
}

// CloseIndexDir
status_t
UserVolume::CloseIndexDir(void* cookie)
{
	return B_BAD_VALUE;
}

// FreeIndexDirCookie
status_t
UserVolume::FreeIndexDirCookie(void* cookie)
{
	return B_BAD_VALUE;
}

// ReadIndexDir
status_t
UserVolume::ReadIndexDir(void* cookie, void* buffer, size_t bufferSize,
	int32 count, int32* countRead)
{
	return B_BAD_VALUE;
}

// RewindIndexDir
status_t
UserVolume::RewindIndexDir(void* cookie)
{
	return B_BAD_VALUE;
}

// CreateIndex
status_t
UserVolume::CreateIndex(const char* name, int type, int flags)
{
	return B_BAD_VALUE;
}

// RemoveIndex
status_t
UserVolume::RemoveIndex(const char* name)
{
	return B_BAD_VALUE;
}

// RenameIndex
status_t
UserVolume::RenameIndex(const char* oldName, const char* newName)
{
	return B_BAD_VALUE;
}

// StatIndex
status_t
UserVolume::StatIndex(const char *name, struct index_info* indexInfo)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- queries -----

// OpenQuery
status_t
UserVolume::OpenQuery(const char* queryString, ulong flags, port_id port,
	long token, void** cookie)
{
	return B_BAD_VALUE;
}

// CloseQuery
status_t
UserVolume::CloseQuery(void* cookie)
{
	return B_BAD_VALUE;
}

// FreeQueryCookie
status_t
UserVolume::FreeQueryCookie(void* cookie)
{
	return B_BAD_VALUE;
}

// ReadQuery
status_t
UserVolume::ReadQuery(void* cookie, void* buffer, size_t bufferSize, int32 count,
	int32* countRead)
{
	return B_BAD_VALUE;
}

