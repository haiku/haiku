//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file bfs.c
	disk_scanner filesystem module for BFS filesystems
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <disk_scanner.h>
#include <fs_info.h>
#include <KernelExport.h>
#include <disk_scanner/fs.h>

#define BFS_FS_MODULE_NAME "disk_scanner/fs/bfs/v1"
const char *kModuleDebugName = "fs/bfs";

#define TRACE(x) ;
//#define TRACE(x) dprintf x

// prototypes
static bool bfs_fs_identify(int deviceFD,
	struct extended_partition_info *partitionInfo, float *priority,
	fs_get_buffer get_buffer, struct fs_buffer_cache *cache);

//----------------------------------------------------------------------
// Stolen from src/add-ons/kernel/file_systems/bfs/bfs.h
//----------------------------------------------------------------------

typedef struct block_run
{
	int32		allocation_group;
	uint16		start;
	uint16		length;
} block_run;

typedef block_run inode_addr;

#define BFS_DISK_NAME_LENGTH	32

typedef struct disk_super_block
{
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
} disk_super_block;

#define SUPER_BLOCK_FS_LENDIAN		'BIGE'		/* BIGE */

#define SUPER_BLOCK_MAGIC1			'BFS1'		/* BFS1 */
#define SUPER_BLOCK_MAGIC2			0xdd121031
#define SUPER_BLOCK_MAGIC3			0x15b6830e

#define SUPER_BLOCK_DISK_CLEAN		'CLEN'		/* CLEN */
#define SUPER_BLOCK_DISK_DIRTY		'DIRT'		/* DIRT */

static
char *
get_tupel(uint32 id)
{
	static unsigned char tupel[5];
	int16 i;

	tupel[0] = 0xff & (id >> 24);
	tupel[1] = 0xff & (id >> 16);
	tupel[2] = 0xff & (id >> 8);
	tupel[3] = 0xff & (id);
	tupel[4] = 0;
	for (i = 0;i < 4;i++)
		if (tupel[i] < ' ' || tupel[i] > 128)
			tupel[i] = '.';

	return (char *)tupel;
}

static
void
dump_super_block(disk_super_block *superBlock)
{
	dprintf("disk_super_block:\n");
	dprintf("  name           = %s\n",superBlock->name);
	dprintf("  magic1         = %#08lx (%s) %s\n",superBlock->magic1, get_tupel(superBlock->magic1), (superBlock->magic1 == SUPER_BLOCK_MAGIC1 ? "valid" : "INVALID"));
	dprintf("  fs_byte_order  = %#08lx (%s)\n",superBlock->fs_byte_order, get_tupel(superBlock->fs_byte_order));
	dprintf("  block_size     = %lu\n",superBlock->block_size);
	dprintf("  block_shift    = %lu\n",superBlock->block_shift);
	dprintf("  num_blocks     = %Lu\n",superBlock->num_blocks);
	dprintf("  used_blocks    = %Lu\n",superBlock->used_blocks);
	dprintf("  inode_size     = %lu\n",superBlock->inode_size);
	dprintf("  magic2         = %#08lx (%s) %s\n",superBlock->magic2, get_tupel(superBlock->magic2), (superBlock->magic2 == (int)SUPER_BLOCK_MAGIC2 ? "valid" : "INVALID"));
	dprintf("  blocks_per_ag  = %lu\n",superBlock->blocks_per_ag);
	dprintf("  ag_shift       = %lu (%lld bytes)\n",superBlock->ag_shift, 1LL << superBlock->ag_shift);
	dprintf("  num_ags        = %lu\n",superBlock->num_ags);
	dprintf("  flags          = %#08lx (%s)\n",superBlock->flags, get_tupel(superBlock->flags));
//	dump_block_run("  log_blocks     = ",superBlock->log_blocks);
	dprintf("  log_start      = %Lu\n",superBlock->log_start);
	dprintf("  log_end        = %Lu\n",superBlock->log_end);
	dprintf("  magic3         = %#08lx (%s) %s\n",superBlock->magic3, get_tupel(superBlock->magic3), (superBlock->magic3 == SUPER_BLOCK_MAGIC3 ? "valid" : "INVALID"));
//	dump_block_run("  root_dir       = ",superBlock->root_dir);
//	dump_block_run("  indices        = ",superBlock->indices);
}

static
inline int64
divide_roundup(int64 num,int32 divisor)
{
	return (num + divisor - 1) / divisor;
}
//----------------------------------------------------------------------
// End stolen BFS code
//----------------------------------------------------------------------

// std_ops
static
status_t
std_ops(int32 op, ...)
{
	TRACE(("%s: std_ops(0x%lx)\n", kModuleDebugName, op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

// read_block
static
status_t
read_block(fs_get_buffer get_buffer, struct fs_buffer_cache *cache,
		   off_t offset, size_t size, uchar **block)
{
	size_t actualSize = 0;
	status_t error = get_buffer(cache, offset, size, (void**)block,
								&actualSize);
	if (error == B_OK && actualSize != size) {
		error = B_ERROR;
		free(*block);
	}
	return error;
}

// bfs_fs_identify
/*! \brief Returns true if the given partition is a valid BFS partition.

	See fs_identify_hook() for more information.

	\todo Fill in partitionInfo->mounted_at with something useful.
*/
static
bool
bfs_fs_identify(int deviceFD, struct extended_partition_info *partitionInfo,
				float *priority, fs_get_buffer get_buffer,
				struct fs_buffer_cache *cache)
{
	bool result = false;
	TRACE(("%s: identify(%d, %p, offset: %lld)\n", kModuleDebugName, deviceFD,
	       partitionInfo, partitionInfo ? partitionInfo->info.offset : -1));

	if (partitionInfo) {
		uchar *buffer = NULL;
		disk_super_block *superBlock = NULL;
		status_t error = read_block(get_buffer, cache, 0, 1024, &buffer);
		if (!error && buffer) {
			superBlock = (disk_super_block*)(buffer+512);
//			dump_super_block(superBlock);

			if (superBlock->magic1 != (int32)SUPER_BLOCK_MAGIC1
				|| superBlock->magic2 != (int32)SUPER_BLOCK_MAGIC2
				|| superBlock->magic3 != (int32)SUPER_BLOCK_MAGIC3
				|| (int32)superBlock->block_size != superBlock->inode_size
				|| superBlock->fs_byte_order != SUPER_BLOCK_FS_LENDIAN
				|| (1UL << superBlock->block_shift) != superBlock->block_size
				|| superBlock->num_ags < 1
				|| superBlock->ag_shift < 1
				|| superBlock->blocks_per_ag < 1
				|| superBlock->num_blocks < 10
				|| superBlock->num_ags != divide_roundup(superBlock->num_blocks,1L << superBlock->ag_shift))
			{
				result = false;
			} else {
				if (partitionInfo->file_system_short_name)
					strcpy(partitionInfo->file_system_short_name, "bfs");
				if (partitionInfo->file_system_long_name)
					strcpy(partitionInfo->file_system_long_name, "Be File System");
				if (partitionInfo->volume_name)
					strcpy(partitionInfo->volume_name, superBlock->name);
				if (priority)
					*priority = 0;
				partitionInfo->file_system_flags = B_FS_IS_PERSISTENT
					| B_FS_HAS_ATTR | B_FS_HAS_MIME | B_FS_HAS_QUERY;
				result = true;
			}
			free(buffer);
		}
	}
	return result;		
}

static fs_module_info bfs_fs_module = 
{
	// module_info
	{
		BFS_FS_MODULE_NAME,
		0,	// better B_KEEP_LOADED?
		std_ops
	},
	bfs_fs_identify
};

_EXPORT fs_module_info *modules[] =
{
	&bfs_fs_module,
	NULL
};


