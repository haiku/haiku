/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXT2_H
#define EXT2_H


#include <ByteOrder.h>
#include <fs_interface.h>


#define EXT2_SUPER_BLOCK_OFFSET	1024

struct ext2_super_block {
	uint32	num_inodes;
	uint32	num_blocks;
	uint32	reserved_blocks;
	uint32	free_blocks;
	uint32	free_inodes;
	uint32	first_data_block;
	uint32	block_shift;
	uint32	fragment_shift;
	uint32	blocks_per_group;
	uint32	fragments_per_group;
	uint32	inodes_per_group;
	uint32	mount_time;
	uint32	write_time;
	uint16	mount_count;
	uint16	max_mount_count;
	uint16	magic;
	uint16	state;
	uint16	error_handling;
	uint16	minor_revision_level;
	uint32	last_check_time;
	uint32	check_interval;
	uint32	creator_os;
	uint32	revision_level;
	uint16	reserved_blocks_uid;
	uint16	reserved_blocks_gid;
	uint32	first_inode;
	uint16	inode_size;
	uint16	block_group;
	uint32	compatible_features;
	uint32	incompatible_features;
	uint32	readonly_features;
	uint8	uuid[16];
	char	name[16];
	char	last_mount_point[64];
	uint32	algorithm_usage_bitmap;
	uint8	preallocated_blocks;
	uint8	preallocated_directory_blocks;
	uint16	_padding;
	
	// journaling ext3 support
	uint8	journal_uuid[16];
	uint32	journal_inode;
	uint32	journal_device;
	uint32	last_orphan;
	uint32	hash_seed[4];
	uint8	default_hash_version;
	uint8	_reserved1;
	uint16	_reserved2;
	uint32	default_mount_options;
	uint32	first_meta_block_group;
	uint32	_reserved3[190];

	uint16 Magic() const { return B_LENDIAN_TO_HOST_INT16(magic); }
	uint32 BlockShift() const { return B_LENDIAN_TO_HOST_INT32(block_shift) + 10; }
	uint32 NumInodes() const { return B_LENDIAN_TO_HOST_INT32(num_inodes); }
	uint32 NumBlocks() const { return B_LENDIAN_TO_HOST_INT32(num_blocks); }
	uint32 FreeInodes() const { return B_LENDIAN_TO_HOST_INT32(free_inodes); }
	uint32 FreeBlocks() const { return B_LENDIAN_TO_HOST_INT32(free_blocks); }
	uint16 InodeSize() const { return B_LENDIAN_TO_HOST_INT16(inode_size); }
	uint32 FirstDataBlock() const
		{ return B_LENDIAN_TO_HOST_INT32(first_data_block); }
	uint32 BlocksPerGroup() const
		{ return B_LENDIAN_TO_HOST_INT32(blocks_per_group); }
	uint32 InodesPerGroup() const
		{ return B_LENDIAN_TO_HOST_INT32(inodes_per_group); }
	uint32 FirstMetaBlockGroup() const
		{ return B_LENDIAN_TO_HOST_INT32(first_meta_block_group); }
	uint32 CompatibleFeatures() const
		{ return B_LENDIAN_TO_HOST_INT32(compatible_features); }
	uint32 ReadOnlyFeatures() const
		{ return B_LENDIAN_TO_HOST_INT32(readonly_features); }
	uint32 IncompatibleFeatures() const
		{ return B_LENDIAN_TO_HOST_INT32(incompatible_features); }

	bool IsValid();
		// implemented in Volume.cpp
} _PACKED;

#define EXT2_OLD_REVISION		0
#define EXT2_DYNAMIC_REVISION	1

// compatible features
#define EXT2_FEATURE_DIRECTORY_PREALLOCATION	0x01
#define EXT2_FEATURE_IMAGIC_INODES				0x02
#define EXT2_FEATURE_HAS_JOURNAL				0x04
#define EXT2_FEATURE_EXT_ATTR					0x08
#define EXT2_FEATURE_RESIZE_INODE				0x10
#define EXT2_FEATURE_DIRECTORY_INDEX			0x20

// read-only compatible features
#define EXT2_READ_ONLY_FEATURE_SPARSE_SUPER		0x01
#define	EXT2_READ_ONLY_FEATURE_LARGE_FILE		0x02
#define EXT2_READ_ONLY_FEATURE_BTREE_DIRECTORY	0x04

// incompatible features
#define EXT2_INCOMPATIBLE_FEATURE_COMPRESSION	0x01
#define EXT2_INCOMPATIBLE_FEATURE_FILE_TYPE		0x02
#define EXT2_INCOMPATIBLE_FEATURE_RECOVER		0x04
#define EXT2_INCOMPATIBLE_FEATURE_JOURNAL		0x08
#define EXT2_INCOMPATIBLE_FEATURE_META_GROUP	0x10

// states
#define EXT2_STATE_VALID						0x01
#define	EXT2_STATE_INVALID						0x02

struct ext2_block_group {
	uint32	block_bitmap;
	uint32	inode_bitmap;
	uint32	inode_table;
	uint16	free_blocks;
	uint16	free_inodes;
	uint16	used_directories;
	uint16	_padding;
	uint32	_reserved[3];

	uint32 InodeTable() const
		{ return B_LENDIAN_TO_HOST_INT32(inode_table); }
};

#define EXT2_DIRECT_BLOCKS			12
#define EXT2_ROOT_NODE				2
#define EXT2_SHORT_SYMLINK_LENGTH	60

struct ext2_inode {
	uint16	mode;
	uint16	uid;
	uint32	size;
	uint32	access_time;
	uint32	creation_time;
	uint32	modification_time;
	uint32	deletion_time;
	uint16	gid;
	uint16	num_links;
	uint32	num_blocks;
	uint32	flags;
	uint32	_reserved1;
	union {
		struct data_stream {
			uint32 direct[EXT2_DIRECT_BLOCKS];
			uint32 indirect;
			uint32 double_indirect;
			uint32 triple_indirect;
		} stream;
		char symlink[EXT2_SHORT_SYMLINK_LENGTH];
	};
	uint32	generation;
	uint32	file_access_control;
	uint32	directory_access_control;
	uint32	fragment;
	uint8	fragment_number;
	uint8	fragment_size;
	uint16	_padding;
	uint16	uid_high;
	uint16	gid_high;
	uint32	_reserved2;

	uint16 Mode() const { return B_LENDIAN_TO_HOST_INT16(mode); }
	uint32 Size() const { return B_LENDIAN_TO_HOST_INT32(size); }
	uint32 Flags() const { return B_LENDIAN_TO_HOST_INT32(flags); }

	time_t AccessTime() const { return B_LENDIAN_TO_HOST_INT32(access_time); }
	time_t CreationTime() const { return B_LENDIAN_TO_HOST_INT32(creation_time); }
	time_t ModificationTime() const { return B_LENDIAN_TO_HOST_INT32(modification_time); }
	time_t DeletionTime() const { return B_LENDIAN_TO_HOST_INT32(deletion_time); }

	uint16 UserID() const
	{
		return B_LENDIAN_TO_HOST_INT16(uid)
			| (B_LENDIAN_TO_HOST_INT16(uid_high) << 16);
	}

	uint16 GroupID() const
	{
		return B_LENDIAN_TO_HOST_INT16(gid)
			| (B_LENDIAN_TO_HOST_INT16(gid_high) << 16);
	}
} _PACKED;

#define EXT2_SUPER_BLOCK_MAGIC			0xef53

// flags
#define EXT2_INODE_SECURE_DELETION		0x00000001
#define EXT2_INODE_UNDELETE				0x00000002
#define EXT2_INODE_COMPRESSED			0x00000004
#define EXT2_INODE_SYNCHRONOUS			0x00000008
#define EXT2_INODE_IMMUTABLE			0x00000010
#define EXT2_INODE_APPEND_ONLY			0x00000020
#define EXT2_INODE_NO_DUMP				0x00000040
#define EXT2_INODE_NO_ACCESS_TIME		0x00000080
#define EXT2_INODE_DIRTY				0x00000100
#define EXT2_INODE_COMPRESSED_BLOCKS	0x00000200
#define EXT2_INODE_DO_NOT_COMPRESS		0x00000400
#define EXT2_INODE_COMPRESSION_ERROR	0x00000800
#define EXT2_INODE_BTREE				0x00001000

#define EXT2_NAME_LENGTH	255

struct ext2_dir_entry {
	uint32	inode_id;
	uint16	length;
	uint8	name_length;
	uint8	file_type;
	char	name[EXT2_NAME_LENGTH];

	uint32 InodeID() const { return B_LENDIAN_TO_HOST_INT32(inode_id); }
	uint16 Length() const { return B_LENDIAN_TO_HOST_INT16(length); }
	uint8 NameLength() const { return name_length; }
	uint8 FileType() const { return file_type; }

	bool IsValid() const
	{
		return Length() > MinimumSize();
			// There is no maximum size, as the last entry spans until the
			// end of the block
	}

	static size_t MinimumSize()
	{
		return sizeof(ext2_dir_entry) - EXT2_NAME_LENGTH;
	}
} _PACKED;

// file types
#define EXT2_TYPE_UNKOWN		0
#define EXT2_TYPE_FILE			1
#define EXT2_TYPE_DIRECTORY		2
#define EXT2_TYPE_CHAR_DEVICE	3
#define EXT2_TYPE_BLOCK_DEVICE	4
#define EXT2_TYPE_FIFO			5
#define EXT2_TYPE_SOCKET		6
#define EXT2_TYPE_SYMLINK		7

extern fs_volume_ops gExt2VolumeOps;
extern fs_vnode_ops gExt2VnodeOps;

#endif	// EXT2_H
