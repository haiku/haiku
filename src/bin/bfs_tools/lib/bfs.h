/* bfs - BFS definitions and helper functions
 *
 * Initial version by Axel Dörfler, axeld@pinc-software.de
 * Parts of this code is based on work previously done by Marcus Overhagen
 *
 * Copyright 2001-2008 pinc Software. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef BFS_H
#define BFS_H


#include <SupportDefs.h>

#if !defined(BEOS_VERSION_DANO) && !defined(__HAIKU__)
#	define B_BAD_DATA B_ERROR
#endif


struct __attribute__((packed)) block_run {
	int32		allocation_group;
	uint16		start;
	uint16		length;
	
	inline bool operator==(const block_run &run) const;
	inline bool operator!=(const block_run &run) const;
	inline bool IsZero() const;
	inline void SetTo(int32 group, uint16 start, uint16 length = 1);

	inline static block_run Run(int32 group, uint16 start, uint16 length = 1);
};

typedef block_run inode_addr;

//**************************************


#define BFS_DISK_NAME_LENGTH	32

struct __attribute__((packed)) disk_super_block
{
	char		name[BFS_DISK_NAME_LENGTH];
	int32		magic1;
	int32		fs_byte_order;
	uint32		block_size;
	uint32		block_shift;
	int64		num_blocks;
	int64		used_blocks;
	int32		inode_size;
	int32		magic2;
	int32		blocks_per_ag;
	int32		ag_shift;
	int32		num_ags;
	int32		flags;
	block_run	log_blocks;
	int64		log_start;
	int64		log_end;
	int32		magic3;
	inode_addr	root_dir;
	inode_addr	indices;
	int32		pad[8];
};

#define SUPER_BLOCK_FS_LENDIAN		'BIGE'		/* BIGE */

#define SUPER_BLOCK_MAGIC1			'BFS1'		/* BFS1 */
#define SUPER_BLOCK_MAGIC2			0xdd121031
#define SUPER_BLOCK_MAGIC3			0x15b6830e

#define SUPER_BLOCK_CLEAN			'CLEN'		/* CLEN */
#define SUPER_BLOCK_DIRTY			'DIRT'		/* DIRT */

//**************************************

#define NUM_DIRECT_BLOCKS			12

struct __attribute__((packed)) data_stream
{
	block_run	direct[NUM_DIRECT_BLOCKS];
	int64		max_direct_range;
	block_run	indirect;
	int64		max_indirect_range;
	block_run	double_indirect;
	int64		max_double_indirect_range;
	int64		size;
};

// **************************************

struct bfs_inode;

struct __attribute__((packed)) small_data
{
	uint32		type;
	uint16		name_size;
	uint16		data_size;
	char		name[0];	// name_size long, followed by data

	inline char		*Name();
	inline uint8	*Data();
	inline small_data *Next();
	inline bool		IsLast(bfs_inode *inode);
};

// the file name is part of the small_data structure
#define FILE_NAME_TYPE			'CSTR'
#define FILE_NAME_NAME			0x13
#define FILE_NAME_NAME_LENGTH	1

// **************************************

#define SHORT_SYMLINK_NAME_LENGTH	144 // length incl. terminating '\0'

struct __attribute__((packed)) bfs_inode
{
	int32		magic1;
	inode_addr	inode_num;
	int32		uid;
	int32		gid;
	int32		mode;				// see sys/stat.h
	int32		flags;
	int64		create_time;
	int64		last_modified_time;
	inode_addr	parent;
	inode_addr	attributes;
	uint32		type;				// attribute type

	int32		inode_size;
	uint32		etc;				// for in-memory structures (unused in Haiku' fs)

	union __attribute__((packed)) {
		data_stream		data;
		char 			short_symlink[SHORT_SYMLINK_NAME_LENGTH];
	};
	int32		pad[4];
	small_data	small_data_start[0];
};

#define INODE_MAGIC1			0x3bbe0ad9
#define INODE_TIME_SHIFT		16
#define INODE_FILE_NAME_LENGTH	256

enum inode_flags
{
	INODE_IN_USE			= 0x00000001,	// always set
	INODE_ATTR_INODE		= 0x00000004,
	INODE_LOGGED			= 0x00000008,	// log changes to the data stream
	INODE_DELETED			= 0x00000010,
	INODE_EMPTY				= 0x00000020,
	INODE_LONG_SYMLINK		= 0x00000040,	// symlink in data stream

	INODE_PERMANENT_FLAGS	= 0x0000ffff,

	INODE_NO_CACHE			= 0x00010000,
	INODE_WAS_WRITTEN		= 0x00020000,
	INODE_NO_TRANSACTION	= 0x00040000
};

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


inline bool block_run::operator==(const block_run &run) const
{
	return allocation_group == run.allocation_group
		&& start == run.start
		&& length == run.length;
}

inline bool block_run::operator!=(const block_run &run) const
{
	return allocation_group != run.allocation_group
		|| start != run.start
		|| length != run.length;
}

inline bool block_run::IsZero() const
{
	return allocation_group == 0 && start == 0 && length == 0;
}

inline void block_run::SetTo(int32 _group,uint16 _start,uint16 _length)
{
	allocation_group = _group;
	start = _start;
	length = _length;
}

inline block_run block_run::Run(int32 group, uint16 start, uint16 length)
{
	block_run run;
	run.allocation_group = group;
	run.start = start;
	run.length = length;
	return run;
}


/************************ small_data inline functions ************************/
//	#pragma mark -


inline char *small_data::Name()
{
	return name;
}

inline uint8 *small_data::Data()
{
	return (uint8 *)name + name_size + 3;
}

inline small_data *small_data::Next()
{
	return (small_data *)((uint8 *)(this + 1) + name_size + 3 + data_size + 1);
}

inline bool small_data::IsLast(bfs_inode *inode)
{
	return (addr_t)this > (addr_t)inode + inode->inode_size - sizeof(small_data)
		   || name_size == 0;
}

#endif	/* BFS_H */
