// Volume.cpp

#include "Volume.h"

#include <new>

#include <AutoLocker.h>

#include "Compatibility.h"
#include "DebugSupport.h"
#include "Node.h"
#include "QueryManager.h"
#include "VolumeManager.h"

// constructor
Volume::Volume(VolumeManager* volumeManager)
	: FSObject(),
	  fLock("volume"),
	  fVolumeManager(volumeManager),
	  fParentVolume(NULL),
	  fName(),
	  fUnmounting(false)
{
}

// destructor
Volume::~Volume()
{
}

// GetVolumeManager
VolumeManager*
Volume::GetVolumeManager() const
{
	return fVolumeManager;
}

// SetParentVolume
void
Volume::SetParentVolume(Volume* parent)
{
	AutoLocker<Locker> _(fLock);
	fParentVolume = parent;
}

// GetParentVolume
Volume*
Volume::GetParentVolume() const
{
	return fParentVolume;
}

// PutVolume
void
Volume::PutVolume()
{
	fVolumeManager->PutVolume(this);
}

// Init
status_t
Volume::Init(const char* name)
{
	if (!name || strlen(name) == 0)
		return B_BAD_VALUE;

	if (!fName.SetTo(name))
		return B_NO_MEMORY;

	return B_OK;
}

// Uninit
void
Volume::Uninit()
{
}

// GetName
const char*
Volume::GetName() const
{
	return fName.GetString();
}

// GetRootID
vnode_id
Volume::GetRootID() const
{
	return GetRootNode()->GetID();
}

// SetUnmounting
void
Volume::SetUnmounting(bool unmounting)
{
	fUnmounting = unmounting;
}

// IsUnmounting
bool
Volume::IsUnmounting() const
{
	return fUnmounting;
}

// HandleEvent
void
Volume::HandleEvent(VolumeEvent* event)
{
}

// PrepareToUnmount
void
Volume::PrepareToUnmount()
{
	fVolumeManager->GetQueryManager()->VolumeUnmounting(this);
}


// #pragma mark -
// #pragma mark ----- client methods -----

// GetVNode
status_t
Volume::GetVNode(vnode_id vnid, Node** node)
{
	return get_vnode(fVolumeManager->GetID(), vnid, (void**)node);
}

// PutVNode
status_t
Volume::PutVNode(vnode_id vnid)
{
	return put_vnode(fVolumeManager->GetID(), vnid);
}

// NewVNode
status_t
Volume::NewVNode(vnode_id vnid, Node* node)
{
	status_t error = new_vnode(fVolumeManager->GetID(), vnid, node);
	if (error == B_OK)
		node->SetKnownToVFS(true);
	return error;
}

// RemoveVNode
status_t
Volume::RemoveVNode(vnode_id vnid)
{
	return remove_vnode(fVolumeManager->GetID(), vnid);
}

// UnremoveVNode
status_t
Volume::UnremoveVNode(vnode_id vnid)
{
	return unremove_vnode(fVolumeManager->GetID(), vnid);
}

// IsVNodeRemoved
int
Volume::IsVNodeRemoved(vnode_id vnid)
{
	return is_vnode_removed(fVolumeManager->GetID(), vnid);
}

// SendNotification
int
Volume::SendNotification(port_id port, int32 token, uint32 what, int32 op,
	nspace_id nsida, nspace_id nsidb, vnode_id vnida, vnode_id vnidb,
	vnode_id vnidc, const char *name)
{
	PRINT("Volume::SendNotification(%ld, %ld, 0x%lx, 0x%lx, %ld, %ld, %lld,"
		" %lld, %lld, \"%s\")\n", port, token, what, op, nsida, nsidb,
		vnida, vnidb, vnidc, name);
	return send_notification(port, token, what, op, nsida, nsidb, vnida,
		vnidb, vnidc, name);
}

// NotifyListener
int
Volume::NotifyListener(int32 opcode, nspace_id nsid, vnode_id vnida,
	vnode_id vnidb, vnode_id vnidc, const char *name)
{
	return notify_listener(opcode, nsid, vnida, vnidb, vnidc, name);
}


// #pragma mark -
// #pragma mark ----- FS -----

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
Volume::WriteFSStat(struct fs_info* info, int32 mask)
{
	return B_BAD_VALUE;
}


// #pragma mark -
// #pragma mark ----- vnodes -----

// ReadVNode
status_t
Volume::ReadVNode(vnode_id vnid, char reenter, Node** _node)
{
	return B_BAD_VALUE;
}

// WriteVNode
status_t
Volume::WriteVNode(Node* node, char reenter)
{
	return B_BAD_VALUE;
}

// RemoveVNode
status_t
Volume::RemoveVNode(Node* node, char reenter)
{
	return B_BAD_VALUE;
}


// #pragma mark -
// #pragma mark ----- nodes -----

// FSync
status_t
Volume::FSync(Node* node)
{
	return B_BAD_VALUE;
}

// ReadStat
status_t
Volume::ReadStat(Node* node, struct stat* st)
{
	return B_BAD_VALUE;
}

// WriteStat
status_t
Volume::WriteStat(Node* node, struct stat *st, uint32 mask)
{
	return B_BAD_VALUE;
}

// Access
status_t
Volume::Access(Node* node, int mode)
{
	return B_BAD_VALUE;
}


// #pragma mark -
// #pragma mark ----- files -----

// Create
status_t
Volume::Create(Node* dir, const char* name, int openMode, int mode,
	vnode_id* vnid, void** cookie)
{
	return B_BAD_VALUE;
}

// Open
status_t
Volume::Open(Node* node, int openMode, void** cookie)
{
	return B_BAD_VALUE;
}

// Close
status_t
Volume::Close(Node* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeCookie
status_t
Volume::FreeCookie(Node* node, void* cookie)
{
	return B_BAD_VALUE;
}

// Read
status_t
Volume::Read(Node* node, void* cookie, off_t pos, void* _buffer,
	size_t bufferSize, size_t* _bytesRead)
{
	return B_BAD_VALUE;
}

// Write
status_t
Volume::Write(Node* node, void* cookie, off_t pos, const void* _buffer,
	size_t bufferSize, size_t* bytesWritten)
{
	return B_BAD_VALUE;
}

// IOCtl
status_t
Volume::IOCtl(Node* node, void* cookie, int cmd, void* buffer,
	size_t bufferSize)
{
	return B_DEV_INVALID_IOCTL;
}

// SetFlags
status_t
Volume::SetFlags(Node* node, void* cookie, int flags)
{
	return B_BAD_VALUE;
}


// #pragma mark -
// #pragma mark ----- hard links / symlinks -----

// Link
status_t
Volume::Link(Node* dir, const char* name, Node* node)
{
	return B_BAD_VALUE;
}

// Unlink
status_t
Volume::Unlink(Node* dir, const char* name)
{
	return B_BAD_VALUE;
}

// Symlink
status_t
Volume::Symlink(Node* dir, const char* name, const char* target)
{
	return B_BAD_VALUE;
}

// ReadLink
status_t
Volume::ReadLink(Node* node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	return B_BAD_VALUE;
}

// Rename
status_t
Volume::Rename(Node* oldDir, const char* oldName, Node* newDir,
	const char* newName)
{
	return B_BAD_VALUE;
}


// #pragma mark -
// #pragma mark ----- directories -----

// MkDir
status_t
Volume::MkDir(Node* dir, const char* name, int mode)
{
	return B_BAD_VALUE;
}

// RmDir
status_t
Volume::RmDir(Node* dir, const char* name)
{
	return B_BAD_VALUE;
}

// OpenDir
status_t
Volume::OpenDir(Node* node, void** _cookie)
{
	return B_BAD_VALUE;
}

// CloseDir
status_t
Volume::CloseDir(Node* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeDirCookie
status_t
Volume::FreeDirCookie(Node* node, void* _cookie)
{
	return B_BAD_VALUE;
}

// ReadDir
status_t
Volume::ReadDir(Node* node, void* _cookie, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	return B_BAD_VALUE;
}

// RewindDir
status_t
Volume::RewindDir(Node* node, void* _cookie)
{
	return B_BAD_VALUE;
}

// Walk
status_t
Volume::Walk(Node* dir, const char* entryName, char** resolvedPath,
	vnode_id* vnid)
{
	return B_BAD_VALUE;
}


// #pragma mark -
// #pragma mark ----- attributes -----

// OpenAttrDir
status_t
Volume::OpenAttrDir(Node* node, void** _cookie)
{
	return B_BAD_VALUE;
}

// CloseAttrDir
status_t
Volume::CloseAttrDir(Node* node, void* cookie)
{
	return B_BAD_VALUE;
}

// FreeAttrDirCookie
status_t
Volume::FreeAttrDirCookie(Node* node, void* _cookie)
{
	return B_BAD_VALUE;
}

// ReadAttrDir
status_t
Volume::ReadAttrDir(Node* node, void* _cookie, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	return B_BAD_VALUE;
}

// RewindAttrDir
status_t
Volume::RewindAttrDir(Node* node, void* _cookie)
{
	return B_BAD_VALUE;
}

// ReadAttr
status_t
Volume::ReadAttr(Node* node, const char* name, int type, off_t pos,
	void* _buffer, size_t bufferSize, size_t* _bytesRead)
{
	return B_BAD_VALUE;
}

// WriteAttr
status_t
Volume::WriteAttr(Node* node, const char* name, int type, off_t pos,
	const void* _buffer, size_t bufferSize, size_t* bytesWritten)
{
	return B_BAD_VALUE;
}

// RemoveAttr
status_t
Volume::RemoveAttr(Node* node, const char* name)
{
	return B_BAD_VALUE;
}

// RenameAttr
status_t
Volume::RenameAttr(Node* node, const char* oldName, const char* newName)
{
	return B_BAD_VALUE;
}

// StatAttr
status_t
Volume::StatAttr(Node* node, const char* name, struct attr_info* attrInfo)
{
	return B_BAD_VALUE;
}


// #pragma mark -
// #pragma mark ----- queries -----

// OpenQuery
status_t
Volume::OpenQuery(const char* queryString, uint32 flags, port_id port,
	int32 token, QueryIterator** iterator)
{
	return B_BAD_VALUE;
}

// FreeQueryIterator
void
Volume::FreeQueryIterator(QueryIterator* iterator)
{
}

// ReadQuery
status_t
Volume::ReadQuery(QueryIterator* iterator, struct dirent* buffer,
	size_t bufferSize, int32 count, int32* countRead)
{
	return B_BAD_VALUE;
}

