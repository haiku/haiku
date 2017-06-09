/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2014, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Inode.h"
#include "BTree.h"
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
	btrfs_key search_key;
	search_key.SetType(BTRFS_KEY_TYPE_INODE_ITEM);
	search_key.SetObjectID(fID);
	search_key.SetOffset(0);

	btrfs_inode* node;
	if (fVolume->FSTree()->FindExact(search_key, (void**)&node) != B_OK) {
		ERROR("Inode::UpdateNodeFromDisk(): Couldn't find inode %"
			B_PRIdINO "\n", fID);
		return B_ENTRY_NOT_FOUND;
	}

	memcpy(&fNode, node, sizeof(btrfs_inode));
	free(node);
	return B_OK;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	// you never have write access to a read-only volume
	if ((accessMode & W_OK) != 0 && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	return check_access_permissions(accessMode, Mode(), (gid_t)fNode.GroupID(),
		(uid_t)fNode.UserID());
}


status_t
Inode::FindBlock(off_t pos, off_t& physical, off_t* _length)
{
	btrfs_key search_key;
	search_key.SetType(BTRFS_KEY_TYPE_EXTENT_DATA);
	search_key.SetObjectID(fID);
	search_key.SetOffset(pos + 1);

	btrfs_extent_data* extent_data;
	status_t status = fVolume->FSTree()->FindPrevious(search_key,
		(void**)&extent_data);
	if (status != B_OK) {
		ERROR("Inode::FindBlock(): Couldn't find extent_data 0x%" B_PRIx32
			"\n", status);
		return status;
	}

	TRACE("Inode::FindBlock(%" B_PRIdINO ") key.Offset() %" B_PRId64 "\n",
		ID(), search_key.Offset());

	off_t diff = pos - search_key.Offset();
	off_t logical = 0;
	if (extent_data->Type() == BTRFS_EXTENT_DATA_REGULAR)
		logical = diff + extent_data->disk_offset;
	else
		panic("unknown extent type; %d\n", extent_data->Type());
	status = fVolume->FindBlock(logical, physical);
	if (_length != NULL)
		*_length = extent_data->Size() - diff;
	TRACE("Inode::FindBlock(%" B_PRIdINO ") %" B_PRIdOFF " physical %"
		B_PRIdOFF "\n", ID(), pos, physical);

	free(extent_data);
	return status;
}


status_t
Inode::ReadAt(off_t pos, uint8* buffer, size_t* _length)
{
	size_t length = *_length;

	// set/check boundaries for pos/length
	if (pos < 0) {
		ERROR("inode %" B_PRIdINO ": ReadAt failed(pos %" B_PRIdOFF
			", length %lu)\n", ID(), pos, length);
		return B_BAD_VALUE;
	}

	if (pos >= Size() || length == 0) {
		TRACE("inode %" B_PRIdINO ": ReadAt 0 (pos %" B_PRIdOFF
			", length %lu)\n", ID(), pos, length);
		*_length = 0;
		return B_NO_ERROR;
	}

	// the file cache doesn't seem to like non block aligned file offset
	// so we avoid the file cache for inline extents
	btrfs_key search_key;
	search_key.SetType(BTRFS_KEY_TYPE_EXTENT_DATA);
	search_key.SetObjectID(fID);
	search_key.SetOffset(pos + 1);

	size_t item_size;
	btrfs_extent_data* extent_data;
	status_t status = fVolume->FSTree()->FindPrevious(search_key,
		(void**)&extent_data, &item_size);
	if (status != B_OK) {
		ERROR("Inode::FindBlock(): Couldn't find extent_data 0x%" B_PRIx32
			"\n", status);
		return status;
	}

	uint8 compression = extent_data->Compression();
	if (FileCache() != NULL
		&& extent_data->Type() == BTRFS_EXTENT_DATA_REGULAR) {
		TRACE("inode %" B_PRIdINO ": ReadAt cache (pos %" B_PRIdOFF ", length %lu)\n",
			ID(), pos, length);
		free(extent_data);
		if (compression == BTRFS_EXTENT_COMPRESS_NONE)
			return file_cache_read(FileCache(), NULL, pos, buffer, _length);
		else if (compression == BTRFS_EXTENT_COMPRESS_ZLIB)
			panic("zlib isn't unsupported for regular extent\n");
		else
			panic("unknown extent compression; %d\n", compression);
	}

	TRACE("Inode::ReadAt(%" B_PRIdINO ") key.Offset() %" B_PRId64 "\n", ID(),
		search_key.Offset());

	off_t diff = pos - search_key.Offset();
	if (extent_data->Type() != BTRFS_EXTENT_DATA_INLINE)
		panic("unknown extent type; %d\n", extent_data->Type());

	*_length = min_c(extent_data->Size() - diff, *_length);
	if (compression == BTRFS_EXTENT_COMPRESS_NONE)
		memcpy(buffer, extent_data->inline_data, *_length);
	else if (compression == BTRFS_EXTENT_COMPRESS_ZLIB) {
		char in[2048];
		z_stream zStream = {
			(Bytef*)in,		// next in
			sizeof(in),		// avail in
			0,				// total in
			NULL,			// next out
			0,				// avail out
			0,				// total out
			0,				// msg
			0,				// state
			Z_NULL,			// zalloc
			Z_NULL,			// zfree
			Z_NULL,			// opaque
			0,				// data type
			0,				// adler
			0,				// reserved
		};

		int status;
		ssize_t offset = 0;
		size_t inline_size = item_size - 13;
		bool headerRead = false;

		TRACE("Inode::ReadAt(%" B_PRIdINO ") diff %" B_PRIdOFF " size %"
			B_PRIuSIZE "\n", ID(), diff, item_size);

		do {
			ssize_t bytesRead = min_c(sizeof(in), inline_size - offset);
			memcpy(in, extent_data->inline_data + offset, bytesRead);
			if (bytesRead != (ssize_t)sizeof(in)) {
				if (bytesRead <= 0) {
					status = Z_STREAM_ERROR;
					break;
				}
			}

			zStream.avail_in = bytesRead;
			zStream.next_in = (Bytef*)in;

			if (!headerRead) {
				headerRead = true;

				zStream.avail_out = length;
				zStream.next_out = (Bytef*)buffer;

				status = inflateInit2(&zStream, 15);
				if (status != Z_OK) {
					free(extent_data);
					return B_ERROR;
				}
			}

			status = inflate(&zStream, Z_SYNC_FLUSH);
			offset += bytesRead;
			if (diff > 0) {
				zStream.next_out -= max_c(bytesRead, diff);
				diff -= max_c(bytesRead, diff);
			}

			if (zStream.avail_in != 0 && status != Z_STREAM_END) {
				TRACE("Inode::ReadAt() didn't read whole block: %s\n",
					zStream.msg);
			}
		} while (status == Z_OK);

		inflateEnd(&zStream);

		if (status != Z_STREAM_END) {
			TRACE("Inode::ReadAt() inflating failed: %d!\n", status);
			free(extent_data);
			return B_BAD_DATA;
		}

		*_length = zStream.total_out;

	} else
		panic("unknown extent compression; %d\n", compression);
	free(extent_data);
	return B_OK;

}


status_t
Inode::FindParent(ino_t* id)
{
	btrfs_key search_key;
	search_key.SetType(BTRFS_KEY_TYPE_INODE_REF);
	search_key.SetObjectID(fID);
	search_key.SetOffset(-1);

	void* node_ref;
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

