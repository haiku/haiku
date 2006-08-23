/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_H_
#define _DOSFS_H_

#if USE_DMALLOC
	#include <dmalloc.h>
#else

/* allocate memory from swappable heap */
#define malloc smalloc
#define free sfree
#define calloc scalloc
#define realloc srealloc

#include <kalloc.h>

#endif

#include <KernelExport.h>

/* for multiple reader/single writer locks */
#define READERS 100000

/* Unfortunately, vnode_id's are defined as signed. This causes problems with
 * programs (notably cp) that use the modulo of a vnode_id (or ino_t) as a
 * hash function to index an array. This means the high bit of every vnode_id
 * is off-limits. Luckily, FAT32 is actually FAT28, so dosfs can make do with
 * only 63 bits.
 */
#define ARTIFICIAL_VNID_BITS	(0x6LL << 60)
#define DIR_CLUSTER_VNID_BITS	(0x4LL << 60)
#define DIR_INDEX_VNID_BITS		0
#define INVALID_VNID_BITS_MASK	(0x9LL << 60)

#define IS_DIR_CLUSTER_VNID(vnid) \
	(((vnid) & ARTIFICIAL_VNID_BITS) == DIR_CLUSTER_VNID_BITS)

#define IS_DIR_INDEX_VNID(vnid) \
	(((vnid) & ARTIFICIAL_VNID_BITS) == DIR_INDEX_VNID_BITS)

#define IS_ARTIFICIAL_VNID(vnid) \
	(((vnid) & ARTIFICIAL_VNID_BITS) == ARTIFICIAL_VNID_BITS)

#define IS_INVALID_VNID(vnid) \
	((!IS_DIR_CLUSTER_VNID((vnid)) && \
	  !IS_DIR_INDEX_VNID((vnid)) && \
	  !IS_ARTIFICIAL_VNID((vnid))) || \
	 ((vnid) & INVALID_VNID_BITS_MASK))

#define GENERATE_DIR_INDEX_VNID(dircluster, index) \
	(DIR_INDEX_VNID_BITS | ((vnode_id)(dircluster) << 32) | (index))

#define GENERATE_DIR_CLUSTER_VNID(dircluster, filecluster) \
	(DIR_CLUSTER_VNID_BITS | ((vnode_id)(dircluster) << 32) | (filecluster))

#define CLUSTER_OF_DIR_CLUSTER_VNID(vnid) \
	((uint32)((vnid) & 0xffffffff))

#define INDEX_OF_DIR_INDEX_VNID(vnid) \
	((uint32)((vnid) & 0xffffffff))

#define DIR_OF_VNID(vnid) \
	((uint32)(((vnid) >> 32) & ~0xf0000000))

#define VNODE_PARENT_DIR_CLUSTER(vnode) \
	CLUSTER_OF_DIR_CLUSTER_VNID((vnode)->dir_vnid)

#include <lock.h>

#define VNODE_MAGIC 'treB'

typedef struct vnode
{
	uint32		magic;
	vnode_id	vnid; 			// self id
	vnode_id 	dir_vnid;		// parent vnode id (directory containing entry)

	uint32		disk_image;		// 0 = no, 1 = BEOS, 2 = IMAGE.BE

	/* iteration is incremented each time the fat chain changes. it's used by
	 * the file read/write code to determine if it needs to retraverse the
	 * fat chain
	 */
	uint32		iteration;

	/* any changes to this block of information should immediately be reflected
	 * on the disk (or at least in the cache) so that get_next_dirent continues
	 * to function properly
	 */
	uint32		sindex, eindex;	// starting and ending index of directory entry	
	uint32		cluster;		// starting cluster of the data
	uint32		mode;			// dos-style attributes
	off_t		st_size;		// in bytes
	time_t		st_time;

	uint32		end_cluster;	// last cluster of the data

	const char *mime;			// mime type (null if none)

	bool		dirty;			// track if vnode had been written to

#if TRACK_FILENAME
	char		*filename;
#endif
} vnode;

// mode bits
#define FAT_READ_ONLY	1
#define FAT_HIDDEN		2
#define FAT_SYSTEM		4
#define FAT_VOLUME		8
#define FAT_SUBDIR		16
#define FAT_ARCHIVE		32

#define NSPACE_MAGIC 'smaI'

struct vcache_entry;

typedef struct _nspace
{
	uint32	magic;
	nspace_id		id;				// ID passed in to fs_mount
	int				fd;				// File descriptor
	char			device[256];
	uint32			flags;			// see <fcntl.be.h> for modes
	
	// info from bpb
	uint32	bytes_per_sector;
	uint32	sectors_per_cluster;
	uint32	reserved_sectors;
	uint32	fat_count;
	uint32	root_entries_count;
	uint32	total_sectors;
	uint32	sectors_per_fat;
	uint8	media_descriptor;
	uint16	fsinfo_sector;

	uint32	total_clusters;			// data clusters, that is
	uint32	free_clusters;
	uint8	fat_bits;
	bool	fat_mirrored;			// true if fat mirroring on
	uint8	active_fat;

	uint32  root_start;				// for fat12 + fat16 only
	uint32	root_sectors;			// for fat12 + fat16 only
	vnode	root_vnode;				// root directory
	int32	vol_entry;				// index in root directory
	char	vol_label[12];			// lfn's need not apply

	uint32	data_start;
	uint32  last_allocated;			// last allocated cluster

	vnode_id beos_vnid;				// vnid of \BEOS directory
	bool	respect_disk_image;

	int		fs_flags;				// flags for this mount

	lock	vlock;					// volume lock

	// vcache state
	struct {
		sem_id vc_sem;
		vnode_id cur_vnid;
		uint32 cache_size;
		struct vcache_entry **by_vnid, **by_loc;
	} vcache;

	struct {
		uint32	entries;
		uint32	allocated;
		vnode_id *vnid_list;
	} dlist;
} nspace;

#define FS_FLAGS_OP_SYNC       0x1
#define FS_FLAGS_LOCK_DOOR     0x2

#define LOCK_VOL(vol) \
	if (vol == NULL) { dprintf("null vol\n"); return EINVAL; } else LOCK((vol)->vlock)

#define UNLOCK_VOL(vol) \
	UNLOCK((vol)->vlock)

#define CHECK_MAGIC(name,struc,magick) \
	int check_##name##_magic(struc *t, char *funcname) \
	{ \
		if (t == NULL) { \
			dprintf("%s passed null " #name " pointer\n", funcname); \
			return EINVAL; \
		} else if (t->magic != magick) { \
			dprintf(#name " (%x) passed to %s has invalid magic number\n", (int)t, funcname); \
			return EINVAL; \
		} else \
			return 0; \
	}

int check_vnode_magic(struct vnode *t, char *funcname);
int check_nspace_magic(struct _nspace *t, char *funcname);

#define TOUCH(x) ((void)(x))

/* debug levels */
extern int debug_attr, debug_dir, debug_dlist, debug_dosfs, debug_encodings,
		debug_fat, debug_file, debug_iter, debug_vcache;

int _dosfs_sync(nspace *vol);

#endif
