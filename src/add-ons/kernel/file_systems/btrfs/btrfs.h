/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BTRFS_H
#define BTRFS_H


#include <sys/stat.h>

#include <ByteOrder.h>
#include <fs_interface.h>
#include <KernelExport.h>


typedef uint64 fileblock_t;		// file block number
typedef uint64 fsblock_t;		// filesystem block number


#define BTRFS_SUPER_BLOCK_OFFSET	0x10000

struct btrfs_key {
	uint64	object_id;
	uint8	type;
	uint64	offset;
	
	uint64	ObjectID() const { return B_LENDIAN_TO_HOST_INT64(object_id); }
	uint8	Type() const { return type; }
	uint64	Offset() const { return B_LENDIAN_TO_HOST_INT64(offset); }
	void SetObjectID(uint64 id) { object_id = B_HOST_TO_LENDIAN_INT64(id); }
	void SetType(uint8 key_type) { type = key_type; }
	void SetOffset(uint64 off) { offset = B_HOST_TO_LENDIAN_INT64(off); }
} _PACKED;

struct btrfs_timespec {
	uint64	seconds;
	uint32	nanoseconds;
} _PACKED;

struct btrfs_header {
	uint8	checksum[32];
	uint8	fsid[16];
	uint64	blocknum;
	uint64	flags;
	uint8	chunk_tree_uuid[16];
	uint64	generation;
	uint64	owner;
	uint32	item_count;
	uint8	level;
	uint64 BlockNum() const { return B_LENDIAN_TO_HOST_INT64(blocknum); }
	uint64 Flags() const { return B_LENDIAN_TO_HOST_INT64(flags); }
	uint64 Generation() const {
		return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 Owner() const {
		return B_LENDIAN_TO_HOST_INT64(owner); }
	uint32 ItemCount() const {
		return B_LENDIAN_TO_HOST_INT32(item_count); }
	uint8 Level() const { return level; }
} _PACKED;

struct btrfs_index {
	btrfs_key key;
	uint64	blocknum;
	uint64	generation;
	uint64 BlockNum() const { return B_LENDIAN_TO_HOST_INT64(blocknum); }
	uint64 Generation() const {
		return B_LENDIAN_TO_HOST_INT64(generation); }
} _PACKED;

struct btrfs_entry {
	btrfs_key key;
	uint32 offset;
	uint32 size;
	uint32 Offset() const {
		return B_LENDIAN_TO_HOST_INT32(offset); }
	uint32 Size() const {
		return B_LENDIAN_TO_HOST_INT32(size); }
} _PACKED;

struct btrfs_stream {
	btrfs_header header;
	union {
		btrfs_entry entries[0];
		btrfs_index index[0];
	};
} _PACKED;

struct btrfs_stripe {
	uint64	device_id;
	uint64	offset;
	uint8	device_uuid[16];
	uint64	DeviceID() const { return B_LENDIAN_TO_HOST_INT64(device_id); }
	uint64	Offset() const { return B_LENDIAN_TO_HOST_INT64(offset); }
} _PACKED;

struct btrfs_chunk {
	uint64	length;
	uint64	owner;
	uint64	stripe_length;
	uint64	type;
	uint32	io_align;
	uint32	io_width;
	uint32	sector_size;
	uint16	stripe_count;
	uint16	sub_stripes;
	struct btrfs_stripe stripes[0];
	uint64 Length() const { return B_LENDIAN_TO_HOST_INT64(length); }
	uint64 Owner() const { return B_LENDIAN_TO_HOST_INT64(owner); }
	uint64 StripeLength() const
		{ return B_LENDIAN_TO_HOST_INT64(stripe_length); }
	uint64 Type() const { return B_LENDIAN_TO_HOST_INT64(type); }
	uint32 IOAlign() const { return B_LENDIAN_TO_HOST_INT32(io_align); }
	uint32 IOWidth() const { return B_LENDIAN_TO_HOST_INT32(io_width); }
	uint32 SectorSize() const
		{ return B_LENDIAN_TO_HOST_INT32(sector_size); }
	uint16 StripeCount() const
		{ return B_LENDIAN_TO_HOST_INT16(stripe_count); }
	uint16 SubStripes() const
		{ return B_LENDIAN_TO_HOST_INT16(sub_stripes); }
} _PACKED;

struct btrfs_device {
	uint64	id;
	uint64	total_size;
	uint64	used_size;
	uint32	io_align;
	uint32	io_width;
	uint32	sector_size;
	uint64	type;
	uint64	generation;
	uint64	start_offset;
	uint32	group;
	uint8	seek_speed;
	uint8	bandwidth;
	uint8	uuid[16];
	uint8	fsid[16];
} _PACKED;


struct btrfs_super_block {
	uint8	checksum[32];
	uint8	fsid[16];
	uint64	blocknum;
	uint64	flags;
	char	magic[8];
	uint64	generation;
	uint64	root;
	uint64	chunk_root;
	uint64	log_root;
	uint64	log_root_transaction_id;
	uint64	total_size;
	uint64	used_size;
	uint64	root_dir_object_id;
	uint64	num_devices;
	uint32	sector_size;
	uint32	node_size;
	uint32	leaf_size;
	uint32	stripe_size;
	uint32	system_chunk_array_size;
	uint64	chunk_root_generation;
	uint64	compat_flags;
	uint64	readonly_flags;
	uint64	incompat_flags;
	uint16	checksum_type;
	uint8	root_level;
	uint8	chunk_root_level;
	uint8	log_root_level;
	struct btrfs_device device;
	char	label[256];
	uint64	reserved[32];
	uint8	system_chunk_array[2048];

	bool IsValid();
		// implemented in Volume.cpp
	uint64 TotalSize() const { return B_LENDIAN_TO_HOST_INT64(total_size); }
	uint32 BlockSize() const { return B_LENDIAN_TO_HOST_INT32(sector_size); }
	uint64 RootDirObjectID() const {
		return B_LENDIAN_TO_HOST_INT64(root_dir_object_id); }
	uint64 Generation() const {
		return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 Root() const {
		return B_LENDIAN_TO_HOST_INT64(root); }
	uint64 ChunkRoot() const {
		return B_LENDIAN_TO_HOST_INT64(chunk_root); }
	uint64 LogRoot() const {
		return B_LENDIAN_TO_HOST_INT64(log_root); }
	uint8 ChunkRootLevel() const { return chunk_root_level; }
} _PACKED;

struct btrfs_inode {
	uint64	generation;
	uint64	transaction_id;
	uint64	size;
	uint64	nbytes;
	uint64	blockgroup;
	uint32	num_links;
	uint32	uid;
	uint32	gid;
	uint32	mode;
	uint64	rdev;
	uint64	flags;
	uint64	sequence;
	uint64	reserved[4];
	struct btrfs_timespec access_time;
	struct btrfs_timespec change_time;
	struct btrfs_timespec modification_time;
	struct btrfs_timespec creation_time;
	uint64 Generation() const { return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 Size() const { return B_LENDIAN_TO_HOST_INT64(size); }
	uint32 UserID() const { return B_LENDIAN_TO_HOST_INT32(uid); }
	uint32 GroupID() const { return B_LENDIAN_TO_HOST_INT32(gid); }
	uint32 Mode() const { return B_LENDIAN_TO_HOST_INT32(mode); }
	uint64 Flags() const { return B_LENDIAN_TO_HOST_INT64(flags); }
	uint64 Sequence() const { return B_LENDIAN_TO_HOST_INT64(sequence); }
	static void _DecodeTime(struct timespec &timespec,
		const struct btrfs_timespec &time)
	{
		timespec.tv_sec = B_LENDIAN_TO_HOST_INT64(time.seconds);
		timespec.tv_nsec = B_LENDIAN_TO_HOST_INT32(time.nanoseconds);
	}
	void GetAccessTime(struct timespec &timespec) const 
		{ _DecodeTime(timespec, access_time); }
	void GetChangeTime(struct timespec &timespec) const 
		{ _DecodeTime(timespec, change_time); }
	void GetModificationTime(struct timespec &timespec) const 
		{ _DecodeTime(timespec, modification_time); }
	void GetCreationTime(struct timespec &timespec) const 
		{ _DecodeTime(timespec, creation_time); }
} _PACKED;

struct btrfs_root {
	btrfs_inode inode;
	uint64	generation;
	uint64	root_dirid;
	uint64	blocknum;
	uint64	limit_bytes;
	uint64	used_bytes;
	uint64	last_snapshot;
	uint64	flags;
	uint32	refs;
	btrfs_key drop_progress;
	uint8	drop_level;
	uint8	level;
	uint64 Generation() const {
		return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 BlockNum() const { return B_LENDIAN_TO_HOST_INT64(blocknum); }
} _PACKED;

struct btrfs_dir_entry {
	btrfs_key location;
	uint64	transaction_id;
	uint16	data_length;
	uint16	name_length;
	uint8	type;
	uint16 DataLength() const { return B_LENDIAN_TO_HOST_INT16(data_length); }
	uint16 NameLength() const { return B_LENDIAN_TO_HOST_INT16(name_length); }
	ino_t InodeID() const { return location.ObjectID(); }
	uint16 Length() const
		{ return sizeof(*this) + NameLength() + DataLength(); }
} _PACKED;

struct btrfs_extent_data {
	uint64	generation;
	uint64	memory_size;
	uint8	compression;
	uint8	encryption;
	uint16	reserved;
	uint8	type;
	union {
		struct {
			uint64	disk_offset;
			uint64	disk_size;
			uint64	extent_offset;
			uint64	size;
		};
		uint8 inline_data[0];
	};
	uint64 Generation() const {
		return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 MemoryBytes() const {
		return B_LENDIAN_TO_HOST_INT64(memory_size); }
	uint8 Compression() const { return compression; }
	uint8 Type() const { return type; }
	uint64 DiskOffset() const {
		return B_LENDIAN_TO_HOST_INT64(disk_offset); }
	uint64 DiskSize() const {
		return B_LENDIAN_TO_HOST_INT64(disk_size); }
	uint64 ExtentOffset() const {
		return B_LENDIAN_TO_HOST_INT64(extent_offset); }
	uint64 Size() const {
		return B_LENDIAN_TO_HOST_INT64(size); }
} _PACKED;


#define BTRFS_SUPER_BLOCK_MAGIC			"_BHRfS_M"

#define BTRFS_OBJECT_ID_ROOT_TREE		1
#define BTRFS_OBJECT_ID_EXTENT_TREE		2
#define BTRFS_OBJECT_ID_DEV_TREE		4
#define BTRFS_OBJECT_ID_FS_TREE			5
#define BTRFS_OBJECT_ID_ROOT_TREE_DIR	6
#define BTRFS_OBJECT_ID_CHECKSUM_TREE	7
#define BTRFS_OBJECT_ID_CHUNK_TREE		256

#define BTRFS_KEY_TYPE_CHUNK_ITEM		228
#define BTRFS_KEY_TYPE_DIR_ITEM			84
#define BTRFS_KEY_TYPE_DIR_INDEX		96
#define BTRFS_KEY_TYPE_EXTENT_DATA		108
#define BTRFS_KEY_TYPE_INODE_ITEM		1
#define BTRFS_KEY_TYPE_INODE_REF		12
#define BTRFS_KEY_TYPE_ROOT_ITEM		132
#define BTRFS_KEY_TYPE_XATTR_ITEM		24

#define BTRFS_EXTENT_COMPRESS_NONE		0
#define BTRFS_EXTENT_COMPRESS_ZLIB		1
#define BTRFS_EXTENT_COMPRESS_LZO		2

#define BTRFS_EXTENT_DATA_INLINE		0
#define BTRFS_EXTENT_DATA_REGULAR		1
#define BTRFS_EXTENT_DATA_PRE			2


struct file_cookie {
	bigtime_t	last_notification;
	off_t		last_size;
	int			open_mode;
};

#define BTRFS_OPEN_MODE_USER_MASK		0x7fffffff

extern fs_volume_ops gBtrfsVolumeOps;
extern fs_vnode_ops gBtrfsVnodeOps;

#endif	// BTRFS_H
