// Volume.cpp

#include "Volume.h"

// constructor
Volume::Volume(FileSystem* fileSystem, nspace_id id)
	: fFileSystem(fileSystem),
	  fID(id)
{
}

// destructor
Volume::~Volume()
{
}

// GetFileSystem
UserlandFS::FileSystem*
Volume::GetFileSystem() const
{
	return fFileSystem;
}

// GetID
nspace_id
Volume::GetID() const
{
	return fID;
}

// #pragma mark -
// #pragma mark ----- FS -----

// Mount
status_t
Volume::Mount(const char* device, ulong flags, const char* parameters,
	int32 len, vnode_id* rootID)
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

// ReadFSStat
status_t
Volume::ReadFSStat(fs_info* info)
{
	return B_BAD_VALUE;
}

// WriteFSStat
status_t
Volume::WriteFSStat(struct fs_info *info, long mask)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- vnodes -----

// ReadVNode
status_t
Volume::ReadVNode(vnode_id vnid, char reenter, void** node)
{
	return B_BAD_VALUE;
}

// WriteVNode
status_t
Volume::WriteVNode(void* node, char reenter)
{
	return B_BAD_VALUE;
}

// RemoveVNode
status_t
Volume::RemoveVNode(void* node, char reenter)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- nodes -----

// FSync
status_t
Volume::FSync(void* node)
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
Volume::WriteStat(void* node, struct stat* st, long mask)
{
	return B_BAD_VALUE;
}

// Access
status_t
Volume::Access(void* node, int mode)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- files -----

// Create
status_t
Volume::Create(void* dir, const char* name, int openMode, int mode,
	vnode_id* vnid, void** cookie)
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

// IOCtl
status_t
Volume::IOCtl(void* node, void* cookie, int command, void *buffer,
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
Volume::Select(void* node, void* cookie, uint8 event, uint32 ref,
	selectsync* sync)
{
	return B_BAD_VALUE;
}

// Deselect
status_t
Volume::Deselect(void* node, void* cookie, uint8 event, selectsync* sync)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

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

// Symlink
status_t
Volume::Symlink(void* dir, const char* name, const char* target)
{
	return B_BAD_VALUE;
}

// ReadLink
status_t
Volume::ReadLink(void* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
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

// #pragma mark -
// #pragma mark ----- directories -----

// MkDir
status_t
Volume::MkDir(void* dir, const char* name, int mode)
{
	return B_BAD_VALUE;
}

// RmDir
status_t
Volume::RmDir(void* dir, const char* name)
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
	int32 count, int32* countRead)
{
	return B_BAD_VALUE;
}

// RewindDir
status_t
Volume::RewindDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// Walk
status_t
Volume::Walk(void* dir, const char* entryName, char** resolvedPath,
	vnode_id* vnid)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- attributes -----

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
Volume::ReadAttrDir(void* node, void* cookie, void* buffer, size_t bufferSize,
	int32 count, int32* countRead)
{
	return B_BAD_VALUE;
}

// RewindAttrDir
status_t
Volume::RewindAttrDir(void* node, void* cookie)
{
	return B_BAD_VALUE;
}

// ReadAttr
status_t
Volume::ReadAttr(void* node, const char* name, int type, off_t pos,
	void* buffer, size_t bufferSize, size_t* bytesRead)
{
	return B_BAD_VALUE;
}

// WriteAttr
status_t
Volume::WriteAttr(void* node, const char* name, int type, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	return B_BAD_VALUE;
}

// RemoveAttr
status_t
Volume::RemoveAttr(void* node, const char* name)
{
	return B_BAD_VALUE;
}

// RenameAttr
status_t
Volume::RenameAttr(void* node, const char* oldName, const char* newName)
{
	return B_BAD_VALUE;
}

// StatAttr
status_t
Volume::StatAttr(void* node, const char* name, struct attr_info* attrInfo)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- indices -----

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
	int32 count, int32* countRead)
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
Volume::CreateIndex(const char* name, int type, int flags)
{
	return B_BAD_VALUE;
}

// RemoveIndex
status_t
Volume::RemoveIndex(const char* name)
{
	return B_BAD_VALUE;
}

// RenameIndex
status_t
Volume::RenameIndex(const char* oldName, const char* newName)
{
	return B_BAD_VALUE;
}

// StatIndex
status_t
Volume::StatIndex(const char *name, struct index_info* indexInfo)
{
	return B_BAD_VALUE;
}

// #pragma mark -
// #pragma mark ----- queries -----

// OpenQuery
status_t
Volume::OpenQuery(const char* queryString, ulong flags, port_id port,
	long token, void** cookie)
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
Volume::ReadQuery(void* cookie, void* buffer, size_t bufferSize, int32 count,
	int32* countRead)
{
	return B_BAD_VALUE;
}

