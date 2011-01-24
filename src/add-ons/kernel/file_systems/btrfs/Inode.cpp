/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Inode.h"

#include <string.h>
#include <stdlib.h>

#include "BPlusTree.h"
#include "CachedBlock.h"
#include "Utility.h"


#undef ASSERT
//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#	define ASSERT(x) { if (!(x)) kernel_debugger("btrfs: assert failed: " #x "\n"); }
#else
#	define TRACE(x...) ;
#	define ASSERT(x) ;
#endif
#define ERROR(x...) dprintf("\33[34mbtrfs:\33[0m " x)


Inode::Inode(Volume* volume, ino_t id)
	:
	fVolume(volume),
	fID(id),
	fCache(NULL),
	fMap(NULL)
{
	rw_lock_init(&fLock, "btrfs inode");

	fInitStatus = UpdateNodeFromDisk();
	if (fInitStatus == B_OK) {
		if (!IsDirectory() && !IsSymLink()) {
			fCache = file_cache_create(fVolume->ID(), ID(), Size());
			fMap = file_map_create(fVolume->ID(), ID(), Size());
		}
	}
}


Inode::Inode(Volume* volume)
	:
	fVolume(volume),
	fID(0),
	fCache(NULL),
	fMap(NULL),
	fInitStatus(B_NO_INIT)
{
	rw_lock_init(&fLock, "btrfs inode");
}


Inode::~Inode()
{
	TRACE("Inode destructor\n");
	file_cache_delete(FileCache());
	file_map_delete(Map());
	TRACE("Inode destructor: Done\n");
}


status_t
Inode::InitCheck()
{
	return fInitStatus;
}


status_t
Inode::UpdateNodeFromDisk()
{
	struct btrfs_key search_key;
	search_key.SetType(BTRFS_KEY_TYPE_INODE_ITEM);
	search_key.SetObjectID(fID);
	search_key.SetOffset(0);

	struct btrfs_inode *node;
	if (fVolume->FSTree()->FindExact(search_key, (void**)&node) != B_OK) {
		ERROR("Inode::UpdateNodeFromDisk(): Couldn't find inode %"
			B_PRIdINO "\n", fID);
		return B_ENTRY_NOT_FOUND;
	}

	memcpy(&fNode, node, sizeof(struct btrfs_inode));
	free(node);
	return B_OK;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	// you never have write access to a read-only volume
	if ((accessMode & W_OK) != 0 && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// get node permissions
	mode_t mode = Mode();
	int userPermissions = (mode & S_IRWXU) >> 6;
	int groupPermissions = (mode & S_IRWXG) >> 3;
	int otherPermissions = mode & S_IRWXO;

	// get the node permissions for this uid/gid
	int permissions = 0;
	uid_t uid = geteuid();
	gid_t gid = getegid();

	if (uid == 0) {
		// user is root
		// root has always read/write permission, but at least one of the
		// X bits must be set for execute permission
		permissions = userPermissions | groupPermissions | otherPermissions
			| R_OK | W_OK;
	} else if (uid == (uid_t)fNode.UserID()) {
		// user is node owner
		permissions = userPermissions;
	} else if (gid == (gid_t)fNode.GroupID()) {
		// user is in owning group
		permissions = groupPermissions;
	} else {
		// user is one of the others
		permissions = otherPermissions;
	}

	return (accessMode & ~permissions) == 0 ? B_OK : B_NOT_ALLOWED;
	return B_OK;
}


status_t
Inode::FindBlock(off_t pos, off_t& physical, off_t *_length)
{
	struct btrfs_key search_key;
	search_key.SetType(BTRFS_KEY_TYPE_EXTENT_DATA);
	search_key.SetObjectID(fID);
	search_key.SetOffset(pos + 1);

	btrfs_extent_data *extent_data;
	status_t status = fVolume->FSTree()->FindPrevious(search_key,
		(void**)&extent_data);
	if (status != B_OK) {
		ERROR("Inode::FindBlock(): Couldn't find extent_data 0x%lx\n", status);
		return status;
	}

	TRACE("Inode::FindBlock(%" B_PRIdINO ") key.Offset() %lld\n", ID(),
		search_key.Offset());

	off_t diff = pos - search_key.Offset();
	off_t logical = 0;
	if (extent_data->Type() == BTRFS_EXTENT_DATA_REGULAR)
		logical = diff + extent_data->disk_offset;
	else
		panic("unknown extent type; %d\n", extent_data->Type());
	status = fVolume->FindBlock(logical, physical);	
	if (_length != NULL)
		*_length = extent_data->Size() - diff;
	TRACE("Inode::FindBlock(%" B_PRIdINO ") %lld physical %lld\n", ID(),
		pos, physical);
	
	free(extent_data);
	return status;
}


status_t
Inode::ReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	size_t length = *_length;

	// set/check boundaries for pos/length
	if (pos < 0) {
		ERROR("inode %" B_PRIdINO ": ReadAt failed(pos %lld, length %lu)\n",
			ID(), pos, length);
		return B_BAD_VALUE;
	}

	if (pos >= Size() || length == 0) {
		TRACE("inode %" B_PRIdINO ": ReadAt 0 (pos %lld, length %lu)\n",
			ID(), pos, length);
		*_length = 0;
		return B_NO_ERROR;
	}

	// the file cache doesn't seem to like non block aligned file offset
	// so we avoid the file cache for inline extents
	struct btrfs_key search_key;
	search_key.SetType(BTRFS_KEY_TYPE_EXTENT_DATA);
	search_key.SetObjectID(fID);
	search_key.SetOffset(pos + 1);

	btrfs_extent_data *extent_data;
	status_t status = fVolume->FSTree()->FindPrevious(search_key,
		(void**)&extent_data);
	if (status != B_OK) {
		ERROR("Inode::FindBlock(): Couldn't find extent_data 0x%lx\n", status);
		return status;
	}

	if (FileCache() != NULL
		&& extent_data->Type() == BTRFS_EXTENT_DATA_REGULAR) {
		TRACE("inode %" B_PRIdINO ": ReadAt cache (pos %lld, length %lu)\n", 
			ID(), pos, length);
		free(extent_data);
		return file_cache_read(FileCache(), NULL, pos, buffer, _length);
	}

	TRACE("Inode::ReadAt(%" B_PRIdINO ") key.Offset() %lld\n", ID(),
		search_key.Offset());

	off_t diff = pos - search_key.Offset();
	if (extent_data->Type() != BTRFS_EXTENT_DATA_INLINE)
		panic("unknown extent type; %d\n", extent_data->Type());

	*_length = min_c(extent_data->MemoryBytes() - diff, *_length);
	memcpy(buffer, extent_data->inline_data, *_length);
	free(extent_data);
	return B_OK;
	
}


status_t
Inode::FindParent(ino_t *id)
{
	struct btrfs_key search_key;
	search_key.SetType(BTRFS_KEY_TYPE_INODE_REF);
	search_key.SetObjectID(fID);
	search_key.SetOffset(-1);

	void *node_ref;
	if (fVolume->FSTree()->FindPrevious(search_key, &node_ref) != B_OK) {
		ERROR("Inode::FindParent(): Couldn't find inode for %" B_PRIdINO "\n",
			fID);
		return B_ERROR;
	}

	free(node_ref);
	*id = search_key.Offset();
	TRACE("Inode::FindParent() for %" B_PRIdINO ": %" B_PRIdINO "\n", fID,
		*id);
	
	return B_OK;
}

