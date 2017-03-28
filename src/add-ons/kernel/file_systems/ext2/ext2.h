/*
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXT2_H
#define EXT2_H


#include <sys/stat.h>

#include <ByteOrder.h>
#include <fs_interface.h>
#include <KernelExport.h>


typedef uint64 fileblock_t;		// file block number
typedef uint64 fsblock_t;		// filesystem block number


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
	uint32	read_only_features;
	uint8	uuid[16];
	char	name[16];
	char	last_mount_point[64];
	uint32	algorithm_usage_bitmap;
	uint8	preallocated_blocks;
	uint8	preallocated_directory_blocks;
	uint16	reserved_gdt_blocks;

	// journaling ext3 support
	uint8	journal_uuid[16];
	uint32	journal_inode;
	uint32	journal_device;
	uint32	last_orphan;
	uint32	hash_seed[4];
	uint8	default_hash_version;
	uint8	_reserved1;
	uint16	group_descriptor_size;
	uint32	default_mount_options;
	uint32	first_meta_block_group;
	uint32	fs_creation_time;
	uint32	journal_inode_backup[17];

	// ext4 support
	uint32	num_blocks_high;
	uint32	reserved_blocks_high;
	uint32	free_blocks_high;
	uint16	min_inode_size;
	uint16	want_inode_size;
	uint32	flags;
	uint16	raid_stride;
	uint16	mmp_interval;
	uint64	mmp_block;
	uint32	raid_stripe_width;
	uint8	groups_per_flex_shift;
	uint8	_reserved3;
	uint16	_reserved4;
	uint32	_reserved5[162];

	uint16 Magic() const { return B_LENDIAN_TO_HOST_INT16(magic); }
	uint16 State() const { return B_LENDIAN_TO_HOST_INT16(state); }
	uint32 RevisionLevel() const { return B_LENDIAN_TO_HOST_INT16(revision_level); }
	uint32 BlockShift() const { return B_LENDIAN_TO_HOST_INT32(block_shift) + 10; }
	uint32 NumInodes() const { return B_LENDIAN_TO_HOST_INT32(num_inodes); }
	uint64 NumBlocks(bool has64bits) const
	{
		uint64 blocks = B_LENDIAN_TO_HOST_INT32(num_blocks);
		if (has64bits)
			blocks |= ((uint64)B_LENDIAN_TO_HOST_INT32(num_blocks_high) << 32);
		return blocks;
	}
	uint32 FreeInodes() const { return B_LENDIAN_TO_HOST_INT32(free_inodes); }
	uint64 FreeBlocks(bool has64bits) const
	{
		uint64 blocks = B_LENDIAN_TO_HOST_INT32(free_blocks);
		if (has64bits)
			blocks |= ((uint64)B_LENDIAN_TO_HOST_INT32(free_blocks_high) << 32);
		return blocks;
	}
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
		{ return B_LENDIAN_TO_HOST_INT32(read_only_features); }
	uint32 IncompatibleFeatures() const
		{ return B_LENDIAN_TO_HOST_INT32(incompatible_features); }
	uint16 ReservedGDTBlocks() const
		{ return B_LENDIAN_TO_HOST_INT16(reserved_gdt_blocks); }
	ino_t  JournalInode() const
		{ return B_LENDIAN_TO_HOST_INT32(journal_inode); }
	ino_t  LastOrphan() const
		{ return (ino_t)B_LENDIAN_TO_HOST_INT32(last_orphan); }
	uint32 HashSeed(uint8 i) const
		{ return B_LENDIAN_TO_HOST_INT32(hash_seed[i]); }
	uint16 GroupDescriptorSize() const
		{ return B_LENDIAN_TO_HOST_INT16(group_descriptor_size); }

	void SetFreeInodes(uint32 freeInodes)
		{ free_inodes = B_HOST_TO_LENDIAN_INT32(freeInodes); }
	void SetFreeBlocks(uint64 freeBlocks, bool has64bits)
	{
		free_blocks = B_HOST_TO_LENDIAN_INT32(freeBlocks & 0xffffffff);
		if (has64bits)
			free_blocks_high = B_HOST_TO_LENDIAN_INT32(freeBlocks >> 32);
	}
	void SetLastOrphan(ino_t id)
		{ last_orphan = B_HOST_TO_LENDIAN_INT32((uint32)id); }
	void SetReadOnlyFeatures(uint32 readOnlyFeatures) const
		{ readOnlyFeatures = B_HOST_TO_LENDIAN_INT32(readOnlyFeatures); }

	bool IsValid();
		// implemented in Volume.cpp
} _PACKED;

#define EXT2_OLD_REVISION		0
#define EXT2_DYNAMIC_REVISION	1

#define EXT2_MAX_REVISION		EXT2_DYNAMIC_REVISION

#define EXT2_FS_STATE_VALID		1	// File system was cleanly unmounted
#define EXT2_FS_STATE_ERROR		2	// File system has errors
#define EXT2_FS_STATE_ORPHAN	3	// Orphans are being recovered

// compatible features
#define EXT2_FEATURE_DIRECTORY_PREALLOCATION	0x0001
#define EXT2_FEATURE_IMAGIC_INODES				0x0002
#define EXT2_FEATURE_HAS_JOURNAL				0x0004
#define EXT2_FEATURE_EXT_ATTR					0x0008
#define EXT2_FEATURE_RESIZE_INODE				0x0010
#define EXT2_FEATURE_DIRECTORY_INDEX			0x0020

// read-only compatible features
#define EXT2_READ_ONLY_FEATURE_SPARSE_SUPER		0x0001
#define EXT2_READ_ONLY_FEATURE_LARGE_FILE		0x0002
#define EXT2_READ_ONLY_FEATURE_BTREE_DIRECTORY	0x0004
#define EXT2_READ_ONLY_FEATURE_HUGE_FILE		0x0008
#define EXT2_READ_ONLY_FEATURE_GDT_CSUM			0x0010
#define EXT2_READ_ONLY_FEATURE_DIR_NLINK		0x0020
#define EXT2_READ_ONLY_FEATURE_EXTRA_ISIZE		0x0040

// incompatible features
#define EXT2_INCOMPATIBLE_FEATURE_COMPRESSION	0x0001
#define EXT2_INCOMPATIBLE_FEATURE_FILE_TYPE		0x0002
#define EXT2_INCOMPATIBLE_FEATURE_RECOVER		0x0004
#define EXT2_INCOMPATIBLE_FEATURE_JOURNAL		0x0008
#define EXT2_INCOMPATIBLE_FEATURE_META_GROUP	0x0010
#define EXT2_INCOMPATIBLE_FEATURE_EXTENTS		0x0040
#define EXT2_INCOMPATIBLE_FEATURE_64BIT			0x0080
#define EXT2_INCOMPATIBLE_FEATURE_MMP			0x0100
#define EXT2_INCOMPATIBLE_FEATURE_FLEX_GROUP	0x0200

// states
#define EXT2_STATE_VALID						0x01
#define	EXT2_STATE_INVALID						0x02

#define EXT2_BLOCK_GROUP_NORMAL_SIZE			32
#define EXT2_BLOCK_GROUP_64BIT_SIZE				64

// block group flags
#define EXT2_BLOCK_GROUP_INODE_UNINIT	0x1
#define EXT2_BLOCK_GROUP_BLOCK_UNINIT	0x2


struct ext2_block_group {
	uint32	block_bitmap;
	uint32	inode_bitmap;
	uint32	inode_table;
	uint16	free_blocks;
	uint16	free_inodes;
	uint16	used_directories;
	uint16	flags;
	uint32	_reserved[2];
	uint16	unused_inodes;
	uint16	checksum;

	// ext4
	uint32	block_bitmap_high;
	uint32	inode_bitmap_high;
	uint32	inode_table_high;
	uint16	free_blocks_high;
	uint16	free_inodes_high;
	uint16	used_directories_high;
	uint16	unused_inodes_high;
	uint32	_reserved2[3];

	fsblock_t BlockBitmap(bool has64bits) const
	{
		uint64 block = B_LENDIAN_TO_HOST_INT32(block_bitmap);
		if (has64bits)
			block |=
				((uint64)B_LENDIAN_TO_HOST_INT32(block_bitmap_high) << 32);
		return block;
	}
	fsblock_t InodeBitmap(bool has64bits) const
	{
		uint64 bitmap = B_LENDIAN_TO_HOST_INT32(inode_bitmap);
		if (has64bits)
			bitmap |=
				((uint64)B_LENDIAN_TO_HOST_INT32(inode_bitmap_high) << 32);
		return bitmap;
	}
	uint64 InodeTable(bool has64bits) const
	{
		uint64 table = B_LENDIAN_TO_HOST_INT32(inode_table);
		if (has64bits)
			table |= ((uint64)B_LENDIAN_TO_HOST_INT32(inode_table_high) << 32);
		return table;
	}
	uint32 FreeBlocks(bool has64bits) const
	{
		uint32 blocks = B_LENDIAN_TO_HOST_INT16(free_blocks);
		if (has64bits)
			blocks |=
				((uint32)B_LENDIAN_TO_HOST_INT16(free_blocks_high) << 16);
		return blocks;
	}
	uint32 FreeInodes(bool has64bits) const
	{
		uint32 inodes = B_LENDIAN_TO_HOST_INT16(free_inodes);
		if (has64bits)
			inodes |=
				((uint32)B_LENDIAN_TO_HOST_INT16(free_inodes_high) << 16);
		return inodes;
	}
	uint32 UsedDirectories(bool has64bits) const
	{
		uint32 dirs = B_LENDIAN_TO_HOST_INT16(used_directories);
		if (has64bits)
			dirs |=
				((uint32)B_LENDIAN_TO_HOST_INT16(used_directories_high) << 16);
		return dirs;
	}
	uint16 Flags() const { return B_LENDIAN_TO_HOST_INT16(flags); }
	uint32 UnusedInodes(bool has64bits) const
	{
		uint32 inodes = B_LENDIAN_TO_HOST_INT16(unused_inodes);
		if (has64bits)
			inodes |=
				((uint32)B_LENDIAN_TO_HOST_INT16(unused_inodes_high) << 16);
		return inodes;
	}


	void SetFreeBlocks(uint32 freeBlocks, bool has64bits)
	{
		free_blocks = B_HOST_TO_LENDIAN_INT16(freeBlocks) & 0xffff;
		if (has64bits)
			free_blocks_high = B_HOST_TO_LENDIAN_INT16(freeBlocks >> 16);
	}

	void SetFreeInodes(uint32 freeInodes, bool has64bits)
	{
		free_inodes = B_HOST_TO_LENDIAN_INT16(freeInodes) & 0xffff;
		if (has64bits)
			free_inodes_high = B_HOST_TO_LENDIAN_INT16(freeInodes >> 16);
	}

	void SetUsedDirectories(uint16 usedDirectories, bool has64bits)
	{
		used_directories = B_HOST_TO_LENDIAN_INT16(usedDirectories& 0xffff);
		if (has64bits)
			used_directories_high =
				B_HOST_TO_LENDIAN_INT16(usedDirectories >> 16);
	}

	void SetFlags(uint16 newFlags)
	{
		flags = B_HOST_TO_LENDIAN_INT16(newFlags);
	}

	void SetUnusedInodes(uint32 unusedInodes, bool has64bits)
	{
		unused_inodes = B_HOST_TO_LENDIAN_INT16(unusedInodes) & 0xffff;
		if (has64bits)
			unused_inodes_high = B_HOST_TO_LENDIAN_INT16(unusedInodes >> 16);
	}
} _PACKED;

#define EXT2_DIRECT_BLOCKS			12
#define EXT2_ROOT_NODE				2
#define EXT2_SHORT_SYMLINK_LENGTH	60

struct ext2_data_stream {
	uint32 direct[EXT2_DIRECT_BLOCKS];
	uint32 indirect;
	uint32 double_indirect;
	uint32 triple_indirect;
} _PACKED;

#define EXT2_EXTENT_MAGIC			0xf30a
#define EXT2_EXTENT_MAX_LENGTH		0x8000

struct ext2_extent_header {
	uint16 magic;
	uint16 num_entries;
	uint16 max_entries;
	uint16 depth;
	uint32 generation;
	bool IsValid() const
	{
		return B_LENDIAN_TO_HOST_INT16(magic) == EXT2_EXTENT_MAGIC;
	}
	uint16 NumEntries() const { return B_LENDIAN_TO_HOST_INT16(num_entries); }
	uint16 MaxEntries() const { return B_LENDIAN_TO_HOST_INT16(max_entries); }
	uint16 Depth() const { return B_LENDIAN_TO_HOST_INT16(depth); }
	uint32 Generation() const { return B_LENDIAN_TO_HOST_INT32(generation); }
	void SetNumEntries(uint16 num)
		{ num_entries = B_HOST_TO_LENDIAN_INT16(num); }
	void SetMaxEntries(uint16 max)
		{ max_entries = B_HOST_TO_LENDIAN_INT16(max); }
	void SetDepth(uint16 _depth)
		{ depth = B_HOST_TO_LENDIAN_INT16(_depth); }
	void SetGeneration(uint32 _generation)
		{ generation = B_HOST_TO_LENDIAN_INT32(_generation); }
} _PACKED;

struct ext2_extent_index {
	uint32 logical_block;
	uint32 physical_block;
	uint16 physical_block_high;
	uint16 _reserved;
	uint32 LogicalBlock() const
		{ return B_LENDIAN_TO_HOST_INT32(logical_block); }
	uint64 PhysicalBlock() const { return B_LENDIAN_TO_HOST_INT32(physical_block)
		| ((uint64)B_LENDIAN_TO_HOST_INT16(physical_block_high) << 32); }
	void SetLogicalBlock(uint32 block) {
		logical_block = B_HOST_TO_LENDIAN_INT32(block); }
	void SetPhysicalBlock(uint64 block) {
		physical_block = B_HOST_TO_LENDIAN_INT32(block & 0xffffffff);
		physical_block_high = B_HOST_TO_LENDIAN_INT16((block >> 32) & 0xffff); }
} _PACKED;

struct ext2_extent_entry {
	uint32 logical_block;
	uint16 length;
	uint16 physical_block_high;
	uint32 physical_block;
	uint32 LogicalBlock() const
		{ return B_LENDIAN_TO_HOST_INT32(logical_block); }
	uint16 Length() const { return B_LENDIAN_TO_HOST_INT16(length) == 0x8000
		? 0x8000 : B_LENDIAN_TO_HOST_INT16(length) & 0x7fff; }
	uint64 PhysicalBlock() const { return B_LENDIAN_TO_HOST_INT32(physical_block)
		| ((uint64)B_LENDIAN_TO_HOST_INT16(physical_block_high) << 32); }
	void SetLogicalBlock(uint32 block) {
		logical_block = B_HOST_TO_LENDIAN_INT32(block); }
	void SetLength(uint16 _length) {
		length = B_HOST_TO_LENDIAN_INT16(_length) & 0x7fff; }
	void SetPhysicalBlock(uint64 block) {
		physical_block = B_HOST_TO_LENDIAN_INT32(block & 0xffffffff);
		physical_block_high = B_HOST_TO_LENDIAN_INT16((block >> 32) & 0xffff); }
} _PACKED;

struct ext2_extent_stream {
	ext2_extent_header extent_header;
	union {
		ext2_extent_entry extent_entries[4];
		ext2_extent_index extent_index[4];
	};
} _PACKED;

#define EXT2_INODE_NORMAL_SIZE		128
#define EXT2_INODE_MAX_LINKS		65000

struct ext2_inode {
	uint16	mode;
	uint16	uid;
	uint32	size;
	uint32	access_time;
	uint32	change_time;
	uint32	modification_time;
	uint32	deletion_time;
	uint16	gid;
	uint16	num_links;
	uint32	num_blocks;
	uint32	flags;
	uint32	version;
	union {
		ext2_data_stream stream;
		char symlink[EXT2_SHORT_SYMLINK_LENGTH];
		ext2_extent_stream extent_stream;
	};
	uint32	generation;
	uint32	file_access_control;
	union {
		// for directories vs. files
		uint32	directory_access_control;
		uint32	size_high;
	};
	uint32	fragment;
	union {
		struct {
			uint8	fragment_number;
			uint8	fragment_size;
		};
		uint16 num_blocks_high;
	};
	uint16	_padding;
	uint16	uid_high;
	uint16	gid_high;
	uint32	_reserved2;

	// extra attributes
	uint16	extra_inode_size;
	uint16	_padding2;
	uint32	change_time_extra;
	uint32	modification_time_extra;
	uint32	access_time_extra;
	uint32	creation_time;
	uint32	creation_time_extra;
	uint32	version_high;

	uint16 Mode() const { return B_LENDIAN_TO_HOST_INT16(mode); }
	uint32 Flags() const { return B_LENDIAN_TO_HOST_INT32(flags); }
	uint16 NumLinks() const { return B_LENDIAN_TO_HOST_INT16(num_links); }
	uint32 NumBlocks() const { return B_LENDIAN_TO_HOST_INT32(num_blocks); }
	uint64 NumBlocks64() const { return B_LENDIAN_TO_HOST_INT32(num_blocks)
		| ((uint64)B_LENDIAN_TO_HOST_INT32(num_blocks_high) << 32); }

	static void _DecodeTime(struct timespec *timespec, uint32 time,
		uint32 time_extra, bool extra)
	{
		timespec->tv_sec = B_LENDIAN_TO_HOST_INT32(time);
		if (extra && sizeof(timespec->tv_sec) > 4)
			timespec->tv_sec |=
				(uint64)(B_LENDIAN_TO_HOST_INT32(time_extra) & 0x2) << 32;
		if (extra)
			timespec->tv_nsec = B_LENDIAN_TO_HOST_INT32(time_extra) >> 2;
		else
			timespec->tv_nsec = 0;
	}

	void GetModificationTime(struct timespec *timespec, bool extra) const
		{ _DecodeTime(timespec, modification_time, modification_time_extra,
			extra); }
	void GetAccessTime(struct timespec *timespec, bool extra) const
		{ _DecodeTime(timespec, access_time, access_time_extra, extra); }
	void GetChangeTime(struct timespec *timespec, bool extra) const
		{ _DecodeTime(timespec, change_time, change_time_extra, extra); }
	void GetCreationTime(struct timespec *timespec, bool extra) const
	{
		if (extra)
			_DecodeTime(timespec, creation_time, creation_time_extra, extra);
		else {
			timespec->tv_sec = 0;
			timespec->tv_nsec = 0;
		}
	}
	time_t DeletionTime() const
		{ return B_LENDIAN_TO_HOST_INT32(deletion_time); }

	static uint32 _EncodeTime(const struct timespec *timespec)
	{
		uint32 time = (timespec->tv_nsec << 2) & 0xfffffffc;
		if (sizeof(timespec->tv_sec) > 4)
			time |= (uint64)timespec->tv_sec >> 32;
		return B_HOST_TO_LENDIAN_INT32(time);
	}

	void SetModificationTime(const struct timespec *timespec, bool extra)
	{
		modification_time = B_HOST_TO_LENDIAN_INT32((uint32)timespec->tv_sec);
		if (extra)
			modification_time_extra = _EncodeTime(timespec);
	}
	void SetAccessTime(const struct timespec *timespec, bool extra)
	{
		access_time = B_HOST_TO_LENDIAN_INT32((uint32)timespec->tv_sec);
		if (extra)
			access_time_extra = _EncodeTime(timespec);
	}
	void SetChangeTime(const struct timespec *timespec, bool extra)
	{
		change_time = B_HOST_TO_LENDIAN_INT32((uint32)timespec->tv_sec);
		if (extra)
			change_time_extra = _EncodeTime(timespec);
	}
	void SetCreationTime(const struct timespec *timespec, bool extra)
	{
		if (extra) {
			creation_time = B_HOST_TO_LENDIAN_INT32((uint32)timespec->tv_sec);
			creation_time_extra =
				B_HOST_TO_LENDIAN_INT32((uint32)timespec->tv_nsec);
		}
	}
	void SetDeletionTime(time_t deletionTime)
	{
		deletion_time = B_HOST_TO_LENDIAN_INT32((uint32)deletionTime);
	}

	ino_t  NextOrphan() const { return (ino_t)DeletionTime(); }

	off_t Size() const
	{
		if (S_ISREG(Mode())) {
			return B_LENDIAN_TO_HOST_INT32(size)
				| ((off_t)B_LENDIAN_TO_HOST_INT32(size_high) << 32);
		}

		return B_LENDIAN_TO_HOST_INT32(size);
	}

	uint32 ExtendedAttributesBlock() const
	{	return B_LENDIAN_TO_HOST_INT32(file_access_control);}

	uint16 ExtraInodeSize() const
		{ return B_LENDIAN_TO_HOST_INT16(extra_inode_size); }

	uint32 UserID() const
	{
		return B_LENDIAN_TO_HOST_INT16(uid)
			| (B_LENDIAN_TO_HOST_INT16(uid_high) << 16);
	}

	uint32 GroupID() const
	{
		return B_LENDIAN_TO_HOST_INT16(gid)
			| (B_LENDIAN_TO_HOST_INT16(gid_high) << 16);
	}

	void SetMode(uint16 newMode)
	{
		mode = B_LENDIAN_TO_HOST_INT16(newMode);
	}

	void UpdateMode(uint16 newMode, uint16 mask)
	{
		SetMode((Mode() & ~mask) | (newMode & mask));
	}

	void ClearFlag(uint32 mask)
	{
		flags &= ~B_HOST_TO_LENDIAN_INT32(mask);
	}

	void SetFlag(uint32 mask)
	{
		flags |= B_HOST_TO_LENDIAN_INT32(mask);
	}

	void SetFlags(uint32 newFlags)
	{
		flags = B_HOST_TO_LENDIAN_INT32(newFlags);
	}

	void SetNumLinks(uint16 numLinks)
	{
		num_links = B_HOST_TO_LENDIAN_INT16(numLinks);
	}

	void SetNumBlocks(uint32 numBlocks)
	{
		num_blocks = B_HOST_TO_LENDIAN_INT32(numBlocks);
	}

	void SetNumBlocks64(uint64 numBlocks)
	{
		num_blocks = B_HOST_TO_LENDIAN_INT32(numBlocks & 0xffffffff);
		num_blocks_high = B_HOST_TO_LENDIAN_INT32(numBlocks >> 32);
	}

	void SetNextOrphan(ino_t id)
	{
		deletion_time = B_HOST_TO_LENDIAN_INT32((uint32)id);
	}

	void SetSize(off_t newSize)
	{
		size = B_HOST_TO_LENDIAN_INT32(newSize & 0xFFFFFFFF);
		if (S_ISREG(Mode()))
			size_high = B_HOST_TO_LENDIAN_INT32(newSize >> 32);
	}

	void SetUserID(uint32 newUID)
	{
		uid = B_HOST_TO_LENDIAN_INT16(newUID & 0xFFFF);
		uid_high = B_HOST_TO_LENDIAN_INT16(newUID >> 16);
	}

	void SetGroupID(uint32 newGID)
	{
		gid = B_HOST_TO_LENDIAN_INT16(newGID & 0xFFFF);
		gid_high = B_HOST_TO_LENDIAN_INT16(newGID >> 16);
	}

	void SetExtendedAttributesBlock(uint32 block)
	{
		file_access_control = B_HOST_TO_LENDIAN_INT32(block);
	}

	void SetExtraInodeSize(uint16 newSize)
	{
		extra_inode_size = B_HOST_TO_LENDIAN_INT16(newSize);
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
#define EXT2_INODE_INDEXED				0x00001000
#define EXT2_INODE_JOURNAL_DATA			0x00004000
#define EXT2_INODE_NO_MERGE_TAIL		0x00008000
#define EXT2_INODE_DIR_SYNCH			0x00010000
#define EXT2_INODE_HUGE_FILE			0x00040000
#define EXT2_INODE_EXTENTS				0x00080000
#define EXT2_INODE_LARGE_EA				0x00200000
#define EXT2_INODE_EOF_BLOCKS			0x00400000
#define EXT2_INODE_INLINE_DATA			0x10000000
#define EXT2_INODE_RESERVED				0x80000000

#define EXT2_INODE_INHERITED (EXT2_INODE_SECURE_DELETION | EXT2_INODE_UNDELETE \
	| EXT2_INODE_COMPRESSED | EXT2_INODE_SYNCHRONOUS | EXT2_INODE_IMMUTABLE \
	| EXT2_INODE_APPEND_ONLY | EXT2_INODE_NO_DUMP | EXT2_INODE_NO_ACCESS_TIME \
	| EXT2_INODE_DO_NOT_COMPRESS | EXT2_INODE_JOURNAL_DATA \
	| EXT2_INODE_NO_MERGE_TAIL | EXT2_INODE_DIR_SYNCH)

#define EXT2_NAME_LENGTH	255

struct ext2_dir_entry {
	uint32	inode_id;
	uint16	length;
	uint8	name_length;
	uint8	file_type;
	char	name[EXT2_NAME_LENGTH];

	uint32	InodeID() const { return B_LENDIAN_TO_HOST_INT32(inode_id); }
	uint16	Length() const { return B_LENDIAN_TO_HOST_INT16(length); }
	uint8	NameLength() const { return name_length; }
	uint8	FileType() const { return file_type; }

	void	SetInodeID(uint32 id) { inode_id = B_HOST_TO_LENDIAN_INT32(id); }

	void	SetLength(uint16 newLength/*uint8 nameLength*/)
	{
		length = B_HOST_TO_LENDIAN_INT16(newLength);
		/*name_length = nameLength;

		if (nameLength % 4 == 0) {
			length = B_HOST_TO_LENDIAN_INT16(
				(short)(nameLength + MinimumSize()));
		} else {
			length = B_HOST_TO_LENDIAN_INT16(
				(short)(nameLength % 4 + 1 + MinimumSize()));
		}*/
	}

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
#define EXT2_TYPE_UNKNOWN		0
#define EXT2_TYPE_FILE			1
#define EXT2_TYPE_DIRECTORY		2
#define EXT2_TYPE_CHAR_DEVICE	3
#define EXT2_TYPE_BLOCK_DEVICE	4
#define EXT2_TYPE_FIFO			5
#define EXT2_TYPE_SOCKET		6
#define EXT2_TYPE_SYMLINK		7

#define EXT2_XATTR_MAGIC		0xea020000
#define EXT2_XATTR_ROUND		((1 << 2) - 1)
#define EXT2_XATTR_NAME_LENGTH	255

#define EXT2_XATTR_INDEX_USER	1

struct ext2_xattr_header {
	uint32	magic;
	uint32	refcount;
	uint32	blocks;		// must be 1 for ext2
	uint32	hash;
	uint32	reserved[4];	// zero

	bool IsValid() const
	{
		return B_LENDIAN_TO_HOST_INT32(magic) == EXT2_XATTR_MAGIC
			&& B_LENDIAN_TO_HOST_INT32(blocks) == 1
			&& refcount <= 1024;
	}

	void Dump() const {
		for (unsigned int i = 0; i < Length(); i++)
			dprintf("%02x ", ((uint8 *)this)[i]);
		dprintf("\n");
	}

	static size_t Length()
	{
		return sizeof(ext2_xattr_header);
	}
};

struct ext2_xattr_entry {
	uint8	name_length;
	uint8	name_index;
	uint16	value_offset;
	uint32	value_block;	// must be zero for ext2
	uint32	value_size;
	uint32	hash;
	char	name[EXT2_XATTR_NAME_LENGTH];

	uint8 NameLength() const { return name_length; }
	uint8 NameIndex() const { return name_index; }
	uint16 ValueOffset() const { return
			B_LENDIAN_TO_HOST_INT16(value_offset); }
	uint32 ValueSize() const { return
			B_LENDIAN_TO_HOST_INT32(value_size); }

	// padded sizes
	uint32 Length() const { return (MinimumSize() + NameLength()
		+ EXT2_XATTR_ROUND) & ~EXT2_XATTR_ROUND; }

	bool IsValid() const
	{
		return NameLength() > 0 && value_block == 0;
			// There is no maximum size, as the last entry spans until the
			// end of the block
	}

	void Dump(bool full=false) const {
		for (unsigned int i = 0; i < (full ? sizeof(this) : MinimumSize()); i++)
			dprintf("%02x ", ((uint8 *)this)[i]);
		dprintf("\n");
	}

	static size_t MinimumSize()
	{
		return sizeof(ext2_xattr_entry) - EXT2_XATTR_NAME_LENGTH;
	}
} _PACKED;


struct file_cookie {
	bigtime_t	last_notification;
	off_t		last_size;
	int			open_mode;
};


#define EXT2_OPEN_MODE_USER_MASK		0x7fffffff

#define INODE_NOTIFICATION_INTERVAL		10000000LL


extern fs_volume_ops gExt2VolumeOps;
extern fs_vnode_ops gExt2VnodeOps;

#endif	// EXT2_H
