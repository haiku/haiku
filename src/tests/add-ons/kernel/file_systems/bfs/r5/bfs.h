#ifndef BFS_H
#define BFS_H
/* bfs - BFS definitions and helper functions
**
** Copyright 2001-2004, Axel Dörfler, axeld@pinc-software.de
** Parts of this code is based on work previously done by Marcus Overhagen
**
** Copyright 2001, pinc Software. All Rights Reserved.
** This file may be used under the terms of the MIT License.
*/


#include <SupportDefs.h>

#include "bfs_endian.h"


#ifndef B_BEOS_VERSION_DANO
#	define B_BAD_DATA B_ERROR
#endif

// ToDo: temporary fix! (missing but public ioctls)
#define IOCTL_FILE_UNCACHED_IO	10000

struct block_run {
	int32		allocation_group;
	uint16		start;
	uint16		length;

	int32 AllocationGroup() const { return BFS_ENDIAN_TO_HOST_INT32(allocation_group); }
	uint16 Start() const { return BFS_ENDIAN_TO_HOST_INT16(start); }
	uint16 Length() const { return BFS_ENDIAN_TO_HOST_INT16(length); }

	inline bool operator==(const block_run &run) const;
	inline bool operator!=(const block_run &run) const;
	inline bool IsZero();
	inline bool MergeableWith(block_run run) const;
	inline void SetTo(int32 group, uint16 start, uint16 length = 1);

	inline static block_run Run(int32 group, uint16 start, uint16 length = 1);
		// can't have a constructor because it's used in a union
} _PACKED;

typedef block_run inode_addr;

// Since the block_run::length field spans 16 bits, the largest number of
// blocks covered by a block_run is 65535 (as long as we don't want to
// break compatibility and take a zero length for 65536).
#define MAX_BLOCK_RUN_LENGTH	65535

//**************************************


#define BFS_DISK_NAME_LENGTH	32

struct disk_super_block {
	char		name[BFS_DISK_NAME_LENGTH];
	int32		magic1;
	int32		fs_byte_order;
	uint32		block_size;
	uint32		block_shift;
	off_t		num_blocks;
	off_t		used_blocks;
	int32		inode_size;
	int32		magic2;
	int32		blocks_per_ag;
	int32		ag_shift;
	int32		num_ags;
	int32		flags;
	block_run	log_blocks;
	off_t		log_start;
	off_t		log_end;
	int32		magic3;
	inode_addr	root_dir;
	inode_addr	indices;
	int32		pad[8];

	int32 Magic1() const { return BFS_ENDIAN_TO_HOST_INT32(magic1); }
	int32 Magic2() const { return BFS_ENDIAN_TO_HOST_INT32(magic2); }
	int32 Magic3() const { return BFS_ENDIAN_TO_HOST_INT32(magic3); }
	int32 ByteOrder() const { return BFS_ENDIAN_TO_HOST_INT32(fs_byte_order); }
	uint32 BlockSize() const { return BFS_ENDIAN_TO_HOST_INT32(block_size); }
	uint32 BlockShift() const { return BFS_ENDIAN_TO_HOST_INT32(block_shift); }
	off_t NumBlocks() const { return BFS_ENDIAN_TO_HOST_INT64(num_blocks); }
	off_t UsedBlocks() const { return BFS_ENDIAN_TO_HOST_INT64(used_blocks); }
	int32 InodeSize() const { return BFS_ENDIAN_TO_HOST_INT32(inode_size); }
	int32 BlocksPerAllocationGroup() const { return BFS_ENDIAN_TO_HOST_INT32(blocks_per_ag); }
	int32 AllocationGroups() const { return BFS_ENDIAN_TO_HOST_INT32(num_ags); }
	int32 AllocationGroupShift() const { return BFS_ENDIAN_TO_HOST_INT32(ag_shift); }
	int32 Flags() const { return BFS_ENDIAN_TO_HOST_INT32(flags); }
	off_t LogStart() const { return BFS_ENDIAN_TO_HOST_INT64(log_start); }
	off_t LogEnd() const { return BFS_ENDIAN_TO_HOST_INT64(log_end); }

	// implemented in Volume.cpp:
	bool IsValid();
	void Initialize(const char *name, off_t numBlocks, uint32 blockSize);
} _PACKED;

#define SUPER_BLOCK_FS_LENDIAN		'BIGE'		/* BIGE */

#define SUPER_BLOCK_MAGIC1			'BFS1'		/* BFS1 */
#define SUPER_BLOCK_MAGIC2			0xdd121031
#define SUPER_BLOCK_MAGIC3			0x15b6830e

#define SUPER_BLOCK_DISK_CLEAN		'CLEN'		/* CLEN */
#define SUPER_BLOCK_DISK_DIRTY		'DIRT'		/* DIRT */

//**************************************

#define NUM_DIRECT_BLOCKS			12

struct data_stream {
	block_run	direct[NUM_DIRECT_BLOCKS];
	off_t		max_direct_range;
	block_run	indirect;
	off_t		max_indirect_range;
	block_run	double_indirect;
	off_t		max_double_indirect_range;
	off_t		size;

	off_t MaxDirectRange() const { return BFS_ENDIAN_TO_HOST_INT64(max_direct_range); }
	off_t MaxIndirectRange() const { return BFS_ENDIAN_TO_HOST_INT64(max_indirect_range); }
	off_t MaxDoubleIndirectRange() const { return BFS_ENDIAN_TO_HOST_INT64(max_double_indirect_range); }
	off_t Size() const { return BFS_ENDIAN_TO_HOST_INT64(size); }
} _PACKED;

// This defines the size of the indirect and double indirect
// blocks. Note: the code may not work correctly at some places
// if this value is changed (it's not tested).
#define NUM_ARRAY_BLOCKS		4
#define ARRAY_BLOCKS_SHIFT		2
#define INDIRECT_BLOCKS_SHIFT	(ARRAY_BLOCKS_SHIFT + ARRAY_BLOCKS_SHIFT)

//**************************************

struct bfs_inode;

struct small_data {
	uint32		type;
	uint16		name_size;
	uint16		data_size;

#if !__MWERKS__ //-- mwcc doesn't support thingy[0], so we patch Name() instead
	char		name[0];	// name_size long, followed by data
#endif

	uint32 Type() const { return BFS_ENDIAN_TO_HOST_INT32(type); }
	uint16 NameSize() const { return BFS_ENDIAN_TO_HOST_INT16(name_size); }
	uint16 DataSize() const { return BFS_ENDIAN_TO_HOST_INT16(data_size); }

	inline char		*Name() const;
	inline uint8	*Data() const;
	inline uint32	Size() const;
	inline small_data *Next() const;
	inline bool		IsLast(const bfs_inode *inode) const;
} _PACKED;

// the file name is part of the small_data structure
#define FILE_NAME_TYPE			'CSTR'
#define FILE_NAME_NAME			0x13 
#define FILE_NAME_NAME_LENGTH	1 


//**************************************

class Volume;

#define SHORT_SYMLINK_NAME_LENGTH	144 // length incl. terminating '\0'

struct bfs_inode {
	int32		magic1;
	inode_addr	inode_num;
	int32		uid;
	int32		gid;
	int32		mode;				// see sys/stat.h
	int32		flags;
	bigtime_t	create_time;
	bigtime_t	last_modified_time;
	inode_addr	parent;
	inode_addr	attributes;
	uint32		type;				// attribute type
	
	int32		inode_size;
	uint32		etc;				// a pointer to the Inode object during construction

	union {
		data_stream		data;
		char 			short_symlink[SHORT_SYMLINK_NAME_LENGTH];
	};
	int32		pad[4];

#if !__MWERKS__
	small_data	small_data_start[0];
#endif
	
	int32 Magic1() const { return BFS_ENDIAN_TO_HOST_INT32(magic1); }
	int32 UserID() const { return BFS_ENDIAN_TO_HOST_INT32(uid); }
	int32 GroupID() const { return BFS_ENDIAN_TO_HOST_INT32(gid); }
	int32 Mode() const { return BFS_ENDIAN_TO_HOST_INT32(mode); }
	int32 Flags() const { return BFS_ENDIAN_TO_HOST_INT32(flags); }
	int32 Type() const { return BFS_ENDIAN_TO_HOST_INT32(type); }
	int32 InodeSize() const { return BFS_ENDIAN_TO_HOST_INT32(inode_size); }
	bigtime_t LastModifiedTime() const { return BFS_ENDIAN_TO_HOST_INT64(last_modified_time); }
	bigtime_t CreateTime() const { return BFS_ENDIAN_TO_HOST_INT64(create_time); }

	inline small_data *SmallDataStart();

	status_t InitCheck(Volume *volume);
		// defined in Inode.cpp
} _PACKED;	

#define INODE_MAGIC1			0x3bbe0ad9
#define INODE_TIME_SHIFT		16
#define INODE_TIME_MASK			0xffff
#define INODE_FILE_NAME_LENGTH	256

enum inode_flags {
	INODE_IN_USE			= 0x00000001,	// always set
	INODE_ATTR_INODE		= 0x00000004,
	INODE_LOGGED			= 0x00000008,	// log changes to the data stream
	INODE_DELETED			= 0x00000010,
	INODE_NOT_READY			= 0x00000020,	// used during Inode construction
	INODE_LONG_SYMLINK		= 0x00000040,	// symlink in data stream

	INODE_PERMANENT_FLAGS	= 0x0000ffff,

	INODE_NO_CACHE			= 0x00010000,
	INODE_WAS_WRITTEN		= 0x00020000,
	INODE_NO_TRANSACTION	= 0x00040000,
	INODE_DONT_FREE_SPACE	= 0x00080000,	// only used by the "chkbfs" functionality
	INODE_CHKBFS_RUNNING	= 0x00200000,
};

//**************************************

struct file_cookie {
	bigtime_t last_notification;
	off_t	last_size;
	int		open_mode;
};

// notify every second if the file size has changed
#define INODE_NOTIFICATION_INTERVAL	1000000LL

//**************************************


inline int32
divide_roundup(int32 num,int32 divisor)
{
	return (num + divisor - 1) / divisor;
}

inline int64
divide_roundup(int64 num,int32 divisor)
{
	return (num + divisor - 1) / divisor;
}

inline int
get_shift(uint64 i)
{
	int c;
	c = 0;
	while (i > 1) {
		i >>= 1;
		c++;
	}
	return c;
}

inline int32
round_up(uint32 data)
{
	// rounds up to the next off_t boundary
	return (data + sizeof(off_t) - 1) & ~(sizeof(off_t) - 1);
}


/************************ block_run inline functions ************************/
//	#pragma mark -


inline bool
block_run::operator==(const block_run &run) const
{
	return allocation_group == run.allocation_group
		&& start == run.start
		&& length == run.length;
}


inline bool
block_run::operator!=(const block_run &run) const
{
	return allocation_group != run.allocation_group
		|| start != run.start
		|| length != run.length;
}


inline bool
block_run::IsZero()
{
	return allocation_group == 0 && start == 0 && length == 0;
}


inline bool 
block_run::MergeableWith(block_run run) const
{
	// 65535 is the maximum allowed run size for BFS
	return allocation_group == run.allocation_group
		&& Start() + Length() == run.Start()
		&& (uint32)Length() + run.Length() <= MAX_BLOCK_RUN_LENGTH;
}


inline void
block_run::SetTo(int32 _group,uint16 _start,uint16 _length)
{
	allocation_group = HOST_ENDIAN_TO_BFS_INT32(_group);
	start = HOST_ENDIAN_TO_BFS_INT16(_start);
	length = HOST_ENDIAN_TO_BFS_INT16(_length);
}


inline block_run
block_run::Run(int32 group, uint16 start, uint16 length)
{
	block_run run;
	run.allocation_group = HOST_ENDIAN_TO_BFS_INT32(group);
	run.start = HOST_ENDIAN_TO_BFS_INT16(start);
	run.length = HOST_ENDIAN_TO_BFS_INT16(length);
	return run;
}


/************************ small_data inline functions ************************/
//	#pragma mark -


inline char *
small_data::Name() const
{
#if __MWERKS__
	return (char *)(uint32(&data_size)+uint32(sizeof(data_size)));
#else
	return const_cast<char *>(name);
#endif
}


inline uint8 *
small_data::Data() const
{
	return (uint8 *)Name() + NameSize() + 3;
}


inline uint32 
small_data::Size() const
{
	return sizeof(small_data) + NameSize() + 3 + DataSize() + 1;
}


inline small_data *
small_data::Next() const
{
	return (small_data *)((uint8 *)this + Size());
}


inline bool
small_data::IsLast(const bfs_inode *inode) const
{
	// we need to check the location first, because if name_size is already beyond
	// the block, we would touch invalid memory (although that can't cause wrong
	// results)
	return (uint32)this > (uint32)inode + inode->InodeSize() - sizeof(small_data) || name_size == 0;
}


/************************ bfs_inode inline functions ************************/
//	#pragma mark -


inline small_data *
bfs_inode::SmallDataStart()
{
#if __MWERKS__
	return (small_data *)(&pad[4] /* last item in pad + sizeof(int32) */);
#else
	return small_data_start;
#endif
}


#endif	/* BFS_H */
