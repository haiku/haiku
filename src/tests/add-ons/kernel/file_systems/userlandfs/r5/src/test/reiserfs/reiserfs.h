/* Copyright 1996-2000 Hans Reiser, see reiserfs/README for licensing
 * and copyright details */
//
// Modified by Ingo Weinhold (bonefish), Jan. 2003:
// Glued from the files reiserfs_fs.h and reiserfs_sb.h,
// adjusted for BeOS and cut what wasn't needed for my purpose.

#ifndef REISER_FS_H
#define REISER_FS_H

#include <SupportDefs.h>

//////////////////////
// from reiserfs_fs.h
//

/* there are two formats of keys: 3.5 and 3.6
 */
#define KEY_FORMAT_3_5 0
#define KEY_FORMAT_3_6 1

/* there are two stat datas */
#define STAT_DATA_V1 0
#define STAT_DATA_V2 1

#define SD_OFFSET  0
#define SD_UNIQUENESS 0
#define DOT_OFFSET 1
#define DOT_DOT_OFFSET 2
#define DIRENTRY_UNIQUENESS 500
#define FIRST_ITEM_OFFSET 1

#define REISERFS_ROOT_OBJECTID 2
#define REISERFS_ROOT_PARENT_OBJECTID 1

//
// there are 5 item types currently
//
#define TYPE_STAT_DATA 0
#define TYPE_INDIRECT 1
#define TYPE_DIRECT 2
#define TYPE_DIRENTRY 3 
#define TYPE_ANY 15 // FIXME: comment is required

#define V1_SD_UNIQUENESS 0
#define V1_INDIRECT_UNIQUENESS 0xfffffffe
#define V1_DIRECT_UNIQUENESS 0xffffffff
#define V1_DIRENTRY_UNIQUENESS 500
#define V1_ANY_UNIQUENESS 555 // FIXME: comment is required


/* hash value occupies bits from 7 up to 30 */
static inline
uint32
offset_hash_value(uint64 offset)
{
	return offset & 0x7fffff80ULL;
}

/* generation number occupies 7 bits starting from 0 up to 6 */
static inline
uint32
offset_generation_number(uint64 offset)
{
	return offset & 0x7fULL;
}

#define MAX_GENERATION_NUMBER  127

//
// directories use this key as well as old files
//
struct offset_v1 {
	uint32	k_offset;
	uint32	k_uniqueness;
} _PACKED;

struct offset_v2 {
#if LITTLE_ENDIAN
	// little endian
	uint64	k_offset:60;
	uint64	k_type: 4;
#else
	// big endian
	uint64	k_type: 4;
	uint64	k_offset:60;
#endif
} _PACKED;


struct key {
	uint32	k_dir_id;    /* packing locality: by default parent
						    directory object id */
	uint32	k_objectid;  /* object identifier */
	union {
		offset_v1	k_offset_v1;
		offset_v2	k_offset_v2;
	} _PACKED u;
} _PACKED;


struct block_head {       
	uint16	blk_level;        /* Level of a block in the tree. */
	uint16	blk_nr_item;      /* Number of keys/items in a block. */
	uint16	blk_free_space;   /* Block free space in bytes. */
	uint16	blk_reserved;
	key		blk_right_delim_key; /* kept only for compatibility */
};

struct disk_child {
	uint32	dc_block_number;              /* Disk child's block number. */
	uint16	dc_size;		            /* Disk child's used space.   */
	uint16	dc_reserved;
};

struct item_head
{
	/* Everything in the tree is found by searching for it based on
	* its key.*/
	struct key ih_key; 	
	union {
		/* The free space in the last unformatted node of an
		indirect item if this is an indirect item.  This
		equals 0xFFFF iff this is a direct item or stat data
		item. Note that the key, not this field, is used to
		determine the item type, and thus which field this
		union contains. */
		uint16 ih_free_space_reserved; 
		/* Iff this is a directory item, this field equals the
		number of directory entries in the directory item. */
		uint16 ih_entry_count; 
	} _PACKED u;
	uint16 ih_item_len;           /* total size of the item body */
	uint16 ih_item_location;      /* an offset to the item body
								   * within the block */
	uint16 ih_version;	     /* 0 for all old items, 2 for new
								ones. Highest bit is set by fsck
								temporary, cleaned after all
								done */
} _PACKED;

struct stat_data {
	uint16	sd_mode;	/* file type, permissions */
	uint16	sd_reserved;
	uint32	sd_nlink;	/* number of hard links */
	uint64	sd_size;	/* file size */
	uint32	sd_uid;		/* owner */
	uint32	sd_gid;		/* group */
	uint32	sd_atime;	/* time of last access */
	uint32	sd_mtime;	/* time file was last modified  */
	uint32	sd_ctime;	/* time inode (stat data) was last changed (except changes to sd_atime and sd_mtime) */
	uint32	sd_blocks;
	union {
		uint32	sd_rdev;
		uint32	sd_generation;
	} _PACKED u;
} _PACKED;

struct stat_data_v1
{
	uint16	sd_mode;	/* file type, permissions */
	uint16	sd_nlink;	/* number of hard links */
	uint16	sd_uid;		/* owner */
	uint16	sd_gid;		/* group */
	uint32	sd_size;	/* file size */
	uint32	sd_atime;	/* time of last access */
	uint32	sd_mtime;	/* time file was last modified  */
	uint32	sd_ctime;	/* time inode (stat data) was last changed (except changes to sd_atime and sd_mtime) */
	union {
		uint32	sd_rdev;
		uint32	sd_blocks;	/* number of blocks file uses */
	} _PACKED u;
	uint32	sd_first_direct_byte; /* first byte of file which is stored
	in a direct item: except that if it
	equals 1 it is a symlink and if it
	equals ~(__u32)0 there is no
	direct item.  The existence of this
	field really grates on me. Let's
	replace it with a macro based on
	sd_size and our tail suppression
	policy.  Someday.  -Hans */
} _PACKED;

struct reiserfs_de_head
{
	uint32	deh_offset;		/* third component of the directory entry key */
	uint32	deh_dir_id;		/* objectid of the parent directory of the object, that is referenced
								by directory entry */
	uint32	deh_objectid;		/* objectid of the object, that is referenced by directory entry */
	uint16	deh_location;		/* offset of name in the whole item */
	uint16	deh_state;		/* whether 1) entry contains stat data (for future), and 2) whether
								entry is hidden (unlinked) */
} _PACKED;

#define DEH_Statdata 0			/* not used now */
#define DEH_Visible 2

/* ReiserFS leaves the first 64k unused, so that partition labels have
   enough space.  If someone wants to write a fancy bootloader that
   needs more than 64k, let us know, and this will be increased in size.
   This number must be larger than than the largest block size on any
   platform, or code will break.  -Hans */
#define REISERFS_DISK_OFFSET_IN_BYTES (64 * 1024)

/* the spot for the super in versions 3.5 - 3.5.10 (inclusive) */
#define REISERFS_OLD_DISK_OFFSET_IN_BYTES (8 * 1024)

#define REISERFS_SUPER_MAGIC_STRING "ReIsErFs"
#define REISER2FS_SUPER_MAGIC_STRING "ReIsEr2Fs"

/*
 * values for s_state field
 */
#define REISERFS_VALID_FS    1
#define REISERFS_ERROR_FS    2


//////////////////////
// from reiserfs_sb.h
//

//
// superblock's field values
//
#define REISERFS_VERSION_0 0 /* undistributed bitmap */
#define REISERFS_VERSION_1 1 /* distributed bitmap and resizer*/
#define REISERFS_VERSION_2 2 /* distributed bitmap, resizer, 64-bit, etc*/
#define UNSET_HASH 0 // read_super will guess about, what hash names
                     // in directories were sorted with
#define TEA_HASH  1
#define YURA_HASH 2
#define R5_HASH   3
#define DEFAULT_HASH R5_HASH

/* this is the on disk superblock */

struct reiserfs_super_block
{
  uint32 s_block_count;
  uint32 s_free_blocks;                  /* free blocks count    */
  uint32 s_root_block;           	/* root block number    */
  uint32 s_journal_block;           	/* journal block number    */
  uint32 s_journal_dev;           	/* journal device number  */

  /* Since journal size is currently a #define in a header file, if 
  ** someone creates a disk with a 16MB journal and moves it to a 
  ** system with 32MB journal default, they will overflow their journal 
  ** when they mount the disk.  s_orig_journal_size, plus some checks
  ** while mounting (inside journal_init) prevent that from happening
  */

				/* great comment Chris. Thanks.  -Hans */

  uint32 s_orig_journal_size; 		
  uint32 s_journal_trans_max ;           /* max number of blocks in a transaction.  */
  uint32 s_journal_block_count ;         /* total size of the journal. can change over time  */
  uint32 s_journal_max_batch ;           /* max number of blocks to batch into a trans */
  uint32 s_journal_max_commit_age ;      /* in seconds, how old can an async commit be */
  uint32 s_journal_max_trans_age ;       /* in seconds, how old can a transaction be */
  uint16 s_blocksize;                   	/* block size           */
  uint16 s_oid_maxsize;			/* max size of object id array, see get_objectid() commentary  */
  uint16 s_oid_cursize;			/* current size of object id array */
  uint16 s_state;                       	/* valid or error       */
  char s_magic[12];                     /* reiserfs magic string indicates that file system is reiserfs */
  uint32 s_hash_function_code;		/* indicate, what hash function is being use to sort names in a directory*/
  uint16 s_tree_height;                  /* height of disk tree */
  uint16 s_bmap_nr;                      /* amount of bitmap blocks needed to address each block of file system */
  uint16 s_version;		/* I'd prefer it if this was a string,
                                   something like "3.6.4", and maybe
                                   16 bytes long mostly unused. We
                                   don't need to save bytes in the
                                   superblock. -Hans */
  uint16 s_reserved;
  uint32 s_inode_generation;
  char s_unused[124] ;			/* zero filled by mkreiserfs */
} _PACKED;

#define SB_SIZE (sizeof(struct reiserfs_super_block))

/* this is the super from 3.5.X, where X >= 10 */
struct reiserfs_super_block_v1
{
  uint32 s_block_count;			/* blocks count         */
  uint32 s_free_blocks;                  /* free blocks count    */
  uint32 s_root_block;           	/* root block number    */
  uint32 s_journal_block;           	/* journal block number    */
  uint32 s_journal_dev;           	/* journal device number  */
  uint32 s_orig_journal_size; 		/* size of the journal on FS creation.  used to make sure they don't overflow it */
  uint32 s_journal_trans_max ;           /* max number of blocks in a transaction.  */
  uint32 s_journal_block_count ;         /* total size of the journal. can change over time  */
  uint32 s_journal_max_batch ;           /* max number of blocks to batch into a trans */
  uint32 s_journal_max_commit_age ;      /* in seconds, how old can an async commit be */
  uint32 s_journal_max_trans_age ;       /* in seconds, how old can a transaction be */
  uint16 s_blocksize;                   	/* block size           */
  uint16 s_oid_maxsize;			/* max size of object id array, see get_objectid() commentary  */
  uint16 s_oid_cursize;			/* current size of object id array */
  uint16 s_state;                       	/* valid or error       */
  char s_magic[16];                     /* reiserfs magic string indicates that file system is reiserfs */
  uint16 s_tree_height;                  /* height of disk tree */
  uint16 s_bmap_nr;                      /* amount of bitmap blocks needed to address each block of file system */
  uint32 s_reserved;
} _PACKED;

#define SB_SIZE_V1 (sizeof(struct reiserfs_super_block_v1))

/* Definitions of reiserfs on-disk properties: */
#define REISERFS_3_5 0
#define REISERFS_3_6 1

#endif	// REISER_FS_H
