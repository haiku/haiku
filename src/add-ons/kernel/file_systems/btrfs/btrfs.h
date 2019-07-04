/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BTRFS_H
#define BTRFS_H


#include "system_dependencies.h"


typedef uint64 fileblock_t;		// file block number
typedef uint64 fsblock_t;		// filesystem block number

#define BTRFS_LABEL_SIZE					256

#define BTRFS_SUPER_BLOCK_OFFSET			0x10000	 // 64KiB
#define BTRFS_RESERVED_SPACE_OFFSET			0x100000 // 1MiB

#define BTRFS_NUM_ROOT_BACKUPS				4

#define BTRFS_CSUM_SIZE						32

struct btrfs_backup_roots {
	uint64	root;
	uint64	root_generation;
	uint64	chunk_root;
	uint64	chunk_root_generation;
	uint64	extent_root;
	uint64	extent_root_generation;
	uint64	fs_root;
	uint64	fs_root_generation;
	uint64	device_root;
	uint64	device_root_generation;
	uint64	csum_root;
	uint64	csum_root_generation;
	uint64	total_size;
	uint64	used_size;
	uint64	num_devices;
	uint8	unused_1[32];
	uint8	root_level;
	uint8	chunk_root_level;
	uint8	extent_root_level;
	uint8	fs_root_level;
	uint8	device_root_level;
	uint8	csum_root_level;
	uint8	unused_2[10];

	uint64 Root() const { return B_LENDIAN_TO_HOST_INT64(root); }
	uint64 RootGen() const
		{ return B_LENDIAN_TO_HOST_INT64(root_generation); }
	uint64 ChunkRoot() const { return B_LENDIAN_TO_HOST_INT64(chunk_root); }
	uint64 ChunkRootGen() const
		{ return B_LENDIAN_TO_HOST_INT64(chunk_root_generation); }
	uint64 ExtentRoot() const { return B_LENDIAN_TO_HOST_INT64(extent_root); }
	uint64 ExtentRootGen() const
		{ return B_LENDIAN_TO_HOST_INT64(extent_root_generation); }
	uint64 FSRoot() const { return B_LENDIAN_TO_HOST_INT64(fs_root); }
	uint64 FSRootGen() const
		{ return B_LENDIAN_TO_HOST_INT64(fs_root_generation); }
	uint64 DeviceRoot() const { return B_LENDIAN_TO_HOST_INT64(device_root); }
	uint64 DeviceRootGen() const
		{ return B_LENDIAN_TO_HOST_INT64(device_root_generation); }
	uint64 CSumRoot() const { return B_LENDIAN_TO_HOST_INT64(csum_root); }
	uint64 CSumRootGen() const
		{ return B_LENDIAN_TO_HOST_INT64(csum_root_generation); }
	uint8 RootLevel() const { return root_level; }
	uint8 ChunkRootLevel() const { return chunk_root_level; }
	uint8 ExtentRootLevel() const { return extent_root_level; }
	uint8 FSRootLevel() const { return fs_root_level; }
	uint8 DeviceRootLevel() const { return device_root_level; }
	uint8 CSumRootLevel() const { return csum_root_level; }
} _PACKED;


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
	int32 Compare(const btrfs_key& key) const;
		// implemented in BTree.cpp
} _PACKED;


struct btrfs_timespec {
	uint64	seconds;
	uint32	nanoseconds;
} _PACKED;


struct btrfs_header {
	uint8	checksum[32];
	uuid_t	fsid;
	uint64	logical_address;
	uint64	flags;
	uuid_t	chunk_tree_uuid;
	uint64	generation;
	uint64	owner;
	uint32	item_count;
	uint8	level;
	uint64 LogicalAddress() const
		{ return B_LENDIAN_TO_HOST_INT64(logical_address); }
	uint64 Flags() const { return B_LENDIAN_TO_HOST_INT64(flags); }
	uint64 Generation() const
		{ return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 Owner() const
		{ return B_LENDIAN_TO_HOST_INT64(owner); }
	uint32 ItemCount() const
		{ return B_LENDIAN_TO_HOST_INT32(item_count); }
	uint8 Level() const { return level; }

	void SetLogicalAddress(uint64 logical)
		{ logical_address = B_HOST_TO_LENDIAN_INT64(logical); }
	void SetGeneration(uint64 gen)
		{ generation = B_HOST_TO_LENDIAN_INT64(gen); }
	void SetItemCount(uint32 itemCount)
		{ item_count = B_HOST_TO_LENDIAN_INT32(itemCount); }
} _PACKED;


struct btrfs_index {
	btrfs_key key;
	uint64	logical_address;
	uint64	generation;
	uint64 LogicalAddress() const
		{ return B_LENDIAN_TO_HOST_INT64(logical_address); }
	uint64 Generation() const
		{ return B_LENDIAN_TO_HOST_INT64(generation); }

	void SetLogicalAddress(uint64 address)
		{ logical_address = B_HOST_TO_LENDIAN_INT64(address); }
	void SetGeneration(uint64 gen)
		{ generation = B_HOST_TO_LENDIAN_INT64(gen); }
} _PACKED;


struct btrfs_entry {
	btrfs_key key;
	uint32 offset;
	uint32 size;
	uint32 Offset() const
		{ return B_LENDIAN_TO_HOST_INT32(offset); }
	uint32 Size() const
		{ return B_LENDIAN_TO_HOST_INT32(size); }
	void SetOffset(uint32 off) { offset = B_HOST_TO_LENDIAN_INT32(off); }
	void SetSize(uint32 itemSize) { size = B_HOST_TO_LENDIAN_INT32(itemSize); }
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
	uuid_t	device_uuid;
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
	btrfs_stripe stripes[0];
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
	uuid_t	uuid;
	uuid_t	fsid;
} _PACKED;


struct btrfs_super_block {
	uint8	checksum[BTRFS_CSUM_SIZE];
	uuid_t	fsid;
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
	btrfs_device device;
	char	label[BTRFS_LABEL_SIZE];
	uint64	reserved[32];
	uint8	system_chunk_array[2048];
	btrfs_backup_roots backup_roots[BTRFS_NUM_ROOT_BACKUPS];

	// implemented in Volume.cpp:
	bool IsValid() const;
	void Initialize(const char* name, off_t numBlocks,
			uint32 blockSize, uint32 sectorSize);
	uint64 TotalSize() const { return B_LENDIAN_TO_HOST_INT64(total_size); }
	uint32 BlockSize() const { return B_LENDIAN_TO_HOST_INT32(node_size); }
	uint32 SectorSize() const { return B_LENDIAN_TO_HOST_INT32(sector_size); }
	uint64 RootDirObjectID() const
		{ return B_LENDIAN_TO_HOST_INT64(root_dir_object_id); }
	uint64 Generation() const
		{ return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 Root() const
		{ return B_LENDIAN_TO_HOST_INT64(root); }
	uint64 ChunkRoot() const
		{ return B_LENDIAN_TO_HOST_INT64(chunk_root); }
	uint64 LogRoot() const
		{ return B_LENDIAN_TO_HOST_INT64(log_root); }
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
	btrfs_timespec access_time;
	btrfs_timespec change_time;
	btrfs_timespec modification_time;
	btrfs_timespec creation_time;
	uint64 Generation() const { return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 Size() const { return B_LENDIAN_TO_HOST_INT64(size); }
	uint32 UserID() const { return B_LENDIAN_TO_HOST_INT32(uid); }
	uint32 GroupID() const { return B_LENDIAN_TO_HOST_INT32(gid); }
	uint32 Mode() const { return B_LENDIAN_TO_HOST_INT32(mode); }
	uint64 Flags() const { return B_LENDIAN_TO_HOST_INT64(flags); }
	uint64 Sequence() const { return B_LENDIAN_TO_HOST_INT64(sequence); }
	static void _DecodeTime(struct timespec& timespec,
		const btrfs_timespec& time)
	{
		timespec.tv_sec = B_LENDIAN_TO_HOST_INT64(time.seconds);
		timespec.tv_nsec = B_LENDIAN_TO_HOST_INT32(time.nanoseconds);
	}
	void GetAccessTime(struct timespec& timespec) const
		{ _DecodeTime(timespec, access_time); }
	void GetChangeTime(struct timespec& timespec) const
		{ _DecodeTime(timespec, change_time); }
	void GetModificationTime(struct timespec& timespec) const
		{ _DecodeTime(timespec, modification_time); }
	void GetCreationTime(struct timespec& timespec) const
		{ _DecodeTime(timespec, creation_time); }
	static void SetTime(btrfs_timespec& time, const struct timespec& timespec)
	{
		time.seconds = B_HOST_TO_LENDIAN_INT64(timespec.tv_sec);
		time.nanoseconds = B_HOST_TO_LENDIAN_INT64(timespec.tv_nsec);
	}
} _PACKED;


struct btrfs_inode_ref {
	uint64	index;
	uint16	name_length;
	uint8	name[];

	uint64 Index() const { return index; }
	uint16 NameLength() const { return B_LENDIAN_TO_HOST_INT16(name_length); }
	uint16 Length() const
		{ return sizeof(btrfs_inode_ref) + NameLength(); }
	void SetName(const char* name, uint16 nameLength)
	{
		name_length = B_HOST_TO_LENDIAN_INT16(nameLength);
		memcpy(this->name, name, nameLength);
	}
} _PACKED;


struct btrfs_root {
	btrfs_inode inode;
	uint64	generation;
	uint64	root_dirid;
	uint64	logical_address;
	uint64	limit_bytes;
	uint64	used_bytes;
	uint64	last_snapshot;
	uint64	flags;
	uint32	refs;
	btrfs_key drop_progress;
	uint8	drop_level;
	uint8	level;
	uint64 Generation() const
		{ return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 LogicalAddress() const
		{ return B_LENDIAN_TO_HOST_INT64(logical_address); }
} _PACKED;


struct btrfs_dir_entry {
	btrfs_key location;
	uint64	transaction_id;
	uint16	data_length;
	uint16	name_length;
	uint8	type;
	uint8	name[];
	// if attribute data exists, it goes here
	uint16 DataLength() const { return B_LENDIAN_TO_HOST_INT16(data_length); }
	uint16 NameLength() const { return B_LENDIAN_TO_HOST_INT16(name_length); }
	ino_t InodeID() const { return location.ObjectID(); }
	uint16 Length() const
		{ return sizeof(*this) + NameLength() + DataLength(); }
	void SetTransactionID(uint64 id)
		{ transaction_id = B_HOST_TO_LENDIAN_INT64(id); }
	void SetAttributeData(void* data, uint16 dataLength)
	{
		data_length = B_HOST_TO_LENDIAN_INT16(dataLength);
		if (data != NULL)
			memcpy(&name[name_length], data, dataLength);
	}
	void SetName(const char* name, uint16 nameLength)
	{
		name_length = B_HOST_TO_LENDIAN_INT16(nameLength);
		memcpy(this->name, name, nameLength);
	}
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
	uint64 Generation() const
		{ return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 MemoryBytes() const
		{ return B_LENDIAN_TO_HOST_INT64(memory_size); }
	uint8 Compression() const { return compression; }
	uint8 Type() const { return type; }
	uint64 DiskOffset() const
		{ return B_LENDIAN_TO_HOST_INT64(disk_offset); }
	uint64 DiskSize() const
		{ return B_LENDIAN_TO_HOST_INT64(disk_size); }
	uint64 ExtentOffset() const
		{ return B_LENDIAN_TO_HOST_INT64(extent_offset); }
	uint64 Size() const
		{ return B_LENDIAN_TO_HOST_INT64(size); }
} _PACKED;


struct btrfs_block_group {
	uint64	used_space;
	uint64	chunk_object_id;
	uint64	flags;

	uint64 UsedSpace() const { return B_LENDIAN_TO_HOST_INT64(used_space); }
	uint64 ChunkObjectID() const
		{ return B_LENDIAN_TO_HOST_INT64(chunk_object_id); }
	uint64 Flags() const { return B_LENDIAN_TO_HOST_INT64(flags); }
} _PACKED;


struct btrfs_extent {
	uint64	refs;
	uint64	generation;
	uint64	flags;

	uint64 RefCount() const { return B_LENDIAN_TO_HOST_INT64(refs); }
	uint64 Generation() const { return B_LENDIAN_TO_HOST_INT64(generation); }
	uint64 Flags() const { return B_LENDIAN_TO_HOST_INT64(flags); }
} _PACKED;


struct btrfs_extent_inline_ref {
	uint8	type;
	uint64	offset;

	uint8 Type() const { return type; }
	uint64 Offset() const { return B_LENDIAN_TO_HOST_INT64(offset); }
} _PACKED;


struct btrfs_extent_data_ref {
	uint64	root_id;
	uint64	inode_id;
	uint64	offset;
	uint32	ref_count;

	uint64 RootID() const { return B_LENDIAN_TO_HOST_INT64(root_id); }
	uint64 InodeID() const { return B_LENDIAN_TO_HOST_INT64(inode_id); }
	uint64 Offset() const { return B_LENDIAN_TO_HOST_INT64(offset);}
	uint32 RefCount() const { return B_LENDIAN_TO_HOST_INT32(ref_count); }
} _PACKED;

#define BTRFS_SUPER_BLOCK_MAGIC				"_BHRfS_M"
#define BTRFS_SUPER_BLOCK_MAGIC_TEMPORARY	"!BHRfS_M"

#define BTRFS_FIRST_SUBVOLUME				256

#define BTRFS_CSUM_TYPE_CRC32				0

#define BTRFS_OBJECT_ID_ROOT_TREE			1
#define BTRFS_OBJECT_ID_EXTENT_TREE			2
#define BTRFS_OBJECT_ID_CHUNK_TREE			3
#define BTRFS_OBJECT_ID_DEV_TREE			4
#define BTRFS_OBJECT_ID_FS_TREE				5
#define BTRFS_OBJECT_ID_ROOT_TREE_DIR		6
#define BTRFS_OBJECT_ID_CHECKSUM_TREE		7
#define BTRFS_OBJECT_ID_FIRST_CHUNK_TREE	256

#define BTRFS_KEY_TYPE_ANY					0
#define BTRFS_KEY_TYPE_INODE_ITEM			1
#define BTRFS_KEY_TYPE_INODE_REF			12
#define BTRFS_KEY_TYPE_XATTR_ITEM			24
#define BTRFS_KEY_TYPE_DIR_ITEM				84
#define BTRFS_KEY_TYPE_DIR_INDEX			96
#define BTRFS_KEY_TYPE_EXTENT_DATA			108
#define BTRFS_KEY_TYPE_ROOT_ITEM			132
#define BTRFS_KEY_TYPE_EXTENT_ITEM			168
#define BTRFS_KEY_TYPE_METADATA_ITEM		169
#define BTRFS_KEY_TYPE_EXTENT_DATA_REF		178
#define BTRFS_KEY_TYPE_BLOCKGROUP_ITEM		192
#define BTRFS_KEY_TYPE_CHUNK_ITEM			228

#define BTRFS_EXTENT_COMPRESS_NONE			0
#define BTRFS_EXTENT_COMPRESS_ZLIB			1
#define BTRFS_EXTENT_COMPRESS_LZO			2
#define BTRFS_EXTENT_DATA_INLINE			0
#define BTRFS_EXTENT_DATA_REGULAR			1
#define BTRFS_EXTENT_DATA_PRE				2
#define BTRFS_EXTENT_FLAG_DATA				1
#define BTRFS_EXTENT_FLAG_TREE_BLOCK		2
#define BTRFS_EXTENT_FLAG_ALLOCATED			4

#define BTRFS_BLOCKGROUP_FLAG_DATA			1
#define BTRFS_BLOCKGROUP_FLAG_SYSTEM		2
#define BTRFS_BLOCKGROUP_FLAG_METADATA		4
#define BTRFS_BLOCKGROUP_FLAG_RAID0			8
#define BTRFS_BLOCKGROUP_FLAG_RAID1			16
#define BTRFS_BLOCKGROUP_FLAG_DUP			32
#define BTRFS_BLOCKGROUP_FLAG_RAID10		64
#define BTRFS_BLOCKGROUP_FLAG_RAID5			128
#define BTRFS_BLOCKGROUP_FLAG_RAID6			256
#define BTRFS_BLOCKGROUP_FLAG_MASK			511

// d_type in struct dirent
#define BTRFS_FILETYPE_UNKNOWN				0
#define BTRFS_FILETYPE_REGULAR				1
#define BTRFS_FILETYPE_DIRECTORY			2
#define BTRFS_FILETYPE_CHRDEV				3	// character device
#define BTRFS_FILETYPE_BLKDEV				4	// block device
#define BTRFS_FILETYPE_FIFO					5	// fifo device
#define BTRFS_FILETYPE_SOCKET				6
#define BTRFS_FILETYPE_SYMLINK				7
#define BTRFS_FILETYPE_XATTR				8	// ondisk but not user-visible


struct file_cookie {
	bigtime_t	last_notification;
	off_t		last_size;
	int			open_mode;
};


#define BTRFS_OPEN_MODE_USER_MASK		0x7fffffff

extern fs_volume_ops gBtrfsVolumeOps;
extern fs_vnode_ops gBtrfsVnodeOps;


#endif	// BTRFS_H
