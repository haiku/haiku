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

#include <ddm_modules.h>
#include <DiskDeviceTypes.h>
#include <KernelExport.h>

// B_PLAIN_C_ERROR:
//static const char *kBFSModuleName = "file_systems/bfs/v1";
#define kBFSModuleName "file_systems/bfs/v1"

const char *kModuleDebugName = "fs/bfs";

#define TRACE(x) ;
//#define TRACE(x) dprintf x

// prototypes
//static bool bfs_fs_identify(int deviceFD,
//	struct extended_partition_info *partitionInfo, float *priority,
//	fs_get_buffer get_buffer, struct fs_buffer_cache *cache);

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

#if 0

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

#endif	// 0


// module
static status_t bfs_std_ops(int32 op, ...);

// scanning
static float bfs_identify_partition(int fd, partition_data *partition,
		void **cookie);
static status_t bfs_scan_partition(int fd, partition_data *partition,
		void *cookie);
static void bfs_free_identify_partition_cookie(partition_data *partition,
		void *cookie);
static void bfs_free_partition_content_cookie(partition_data *partition);

static fs_module_info bfs_module = {
	{
		kBFSModuleName,
		0,
		bfs_std_ops
	},
// B_PLAIN_C_ERROR:
//	kPartitionTypeBFS,					// pretty_name
	"BFS Filesystem",					// pretty_name
	B_DISK_SYSTEM_IS_FILE_SYSTEM,		// flags

	// scanning
	bfs_identify_partition,				// identify_partition
	bfs_scan_partition,					// scan_partition
	bfs_free_identify_partition_cookie,	// free_identify_partition_cookie
	bfs_free_partition_content_cookie,	// free_partition_content_cookie

	// querying
	NULL,								// supports_defragmenting_partition
	NULL,								// supports_reparing_partition
	NULL,								// supports_resizing_partition
	NULL,								// supports_moving_partition
	NULL,								// supports_setting_content_name
	NULL,								// supports_initializing_partition
	NULL,								// is_sub_system_for

	NULL,								// validate_resize_partition
	NULL,								// validate_move_partition
	NULL,								// validate_set_content_name
	NULL,								// validate_initialize_partition
	NULL,
								// validate_set_partition_content_parameters

	// writing
	NULL,								// defragment_partition
	NULL,								// repair_partition
	NULL,								// resize_partition
	NULL,								// move_partition
	NULL,								// set_content_name
	NULL,								// initialize_partition
	NULL,								// set_partition_content_parameters
};


#ifdef __cplusplus
extern "C"
#endif
fs_module_info *modules[];
_EXPORT fs_module_info *modules[] =
{
	&bfs_module,
	NULL
};


// read_super_block
static
status_t 
read_super_block(int fd, disk_super_block **_superBlock)
{
	status_t error = B_OK;
	ssize_t bytesRead;
	// allocate space for the super block
	disk_super_block *superBlock
		= (disk_super_block*)malloc(sizeof(disk_super_block));
	if (!superBlock)
		return B_NO_MEMORY;
	// read and check the super block
	bytesRead = read_pos(fd, 512, superBlock, sizeof(disk_super_block));
	if (bytesRead < 0) {
		error = bytesRead;
	} else if (bytesRead != sizeof(disk_super_block)
		|| superBlock->magic1 != (int32)SUPER_BLOCK_MAGIC1
		|| superBlock->magic2 != (int32)SUPER_BLOCK_MAGIC2
		|| superBlock->magic3 != (int32)SUPER_BLOCK_MAGIC3
		|| (int32)superBlock->block_size != superBlock->inode_size
		|| superBlock->fs_byte_order != SUPER_BLOCK_FS_LENDIAN
		|| (1UL << superBlock->block_shift) != superBlock->block_size
		|| superBlock->num_ags < 1
		|| superBlock->ag_shift < 1
		|| superBlock->blocks_per_ag < 1
		|| superBlock->num_blocks < 10
		|| superBlock->num_ags != divide_roundup(superBlock->num_blocks,
												 1L << superBlock->ag_shift)) {
		error = B_ERROR;
	}
	// set result / cleanup on failure
	if (error == B_OK)
		*_superBlock = superBlock;
	else if (superBlock)
		free(superBlock);
	return error;
}

// bfs_std_ops
static
status_t
bfs_std_ops(int32 op, ...)
{
	TRACE(("bfs: bfs_std_ops(0x%lx)\n", op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

// bfs_identify_partition
static
float
bfs_identify_partition(int fd, partition_data *partition, void **cookie)
{
	disk_super_block *superBlock = NULL;
	// check parameters
	if (fd < 0 || !partition || !cookie)
		return -1;
	TRACE(("bfs: bfs_identify_partition(%d, %ld: %lld, %lld, %ld)\n", fd,
		   partition->id, partition->offset, partition->size,
		   partition->block_size));
	// read super block
	if (read_super_block(fd, &superBlock) != B_OK)
		return -1;
	*cookie = superBlock;
	return 0.5;
}

// bfs_scan_partition
static
status_t
bfs_scan_partition(int fd, partition_data *partition, void *cookie)
{
	disk_super_block *superBlock = NULL;
	// check parameters
	if (fd < 0 || !partition || !cookie)
		return B_ERROR;
	TRACE(("bfs: bfs_scan_partition(%d, %ld: %lld, %lld, %ld)\n", fd,
		   partition->id, partition->offset, partition->size,
		   partition->block_size));
	superBlock = (disk_super_block*)cookie;
	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->block_size = superBlock->block_size;
	partition->content_name = strdup(superBlock->name);
	partition->content_type = strdup(kPartitionTypeBFS);
	// (no content_parameters, content_cookie ?)
	// free the super block
	free(superBlock);
	if (!partition->content_name || !partition->content_type)
		return B_NO_MEMORY;
	return B_OK;
}

// bfs_free_identify_partition_cookie
static
void
bfs_free_identify_partition_cookie(partition_data *partition, void *cookie)
{
	if (cookie)
		free(cookie);
}

// bfs_free_partition_content_cookie
static
void
bfs_free_partition_content_cookie(partition_data *partition)
{
	if (partition)
		partition->content_cookie = NULL;
}

