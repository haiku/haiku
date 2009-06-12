/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_SYS_STAT_H_
#define _FSSH_SYS_STAT_H_


#include "fssh_defs.h"
#include "fssh_time.h"


struct fssh_stat {
	fssh_dev_t			fssh_st_dev;		/* "device" that this file resides on */
	fssh_ino_t			fssh_st_ino;		/* this file's inode #, unique per device */
	fssh_mode_t			fssh_st_mode;		/* mode bits (rwx for user, group, etc) */
	fssh_nlink_t		fssh_st_nlink;		/* number of hard links to this file */
	fssh_uid_t			fssh_st_uid;		/* user id of the owner of this file */
	fssh_gid_t			fssh_st_gid;		/* group id of the owner of this file */
	fssh_off_t			fssh_st_size;		/* size in bytes of this file */
	fssh_dev_t			fssh_st_rdev;		/* device type (not used) */
	fssh_size_t			fssh_st_blksize;	/* preferred block size for i/o */
	fssh_timespec		fssh_st_atim;		/* last access time */
	fssh_timespec		fssh_st_mtim;		/* last modification time */
	fssh_timespec		fssh_st_ctim;		/* last change time, not creation time */
	fssh_timespec		fssh_st_crtim;		/* creation time */

	// Haiku extensions:
	// TODO: we might also define special types for files and TTYs
	// TODO: we should find another solution for this, as BStatable::GetStat()
	//		can only retrieve the R5 stat structure
	unsigned int		fssh_st_type;		/* attribute/index type */
	fssh_off_t			fssh_st_blocks;		/* number of blocks allocated for object */
};
typedef struct fssh_stat fssh_struct_stat;

/* compatibility with older apps */
#define fssh_st_atime	fssh_st_atim.tv_sec
#define fssh_st_mtime	fssh_st_mtim.tv_sec
#define fssh_st_ctime	fssh_st_ctim.tv_sec
#define fssh_st_crtime	fssh_st_crtim.tv_sec

/* extended file types */
#define FSSH_S_ATTR_DIR			01000000000	/* attribute directory */
#define FSSH_S_ATTR				02000000000	/* attribute */
#define FSSH_S_INDEX_DIR		04000000000	/* index (or index directory) */
#define FSSH_S_STR_INDEX		00100000000	/* string index */
#define FSSH_S_INT_INDEX		00200000000	/* int32 index */
#define FSSH_S_UINT_INDEX		00400000000	/* uint32 index */
#define FSSH_S_LONG_LONG_INDEX	00010000000	/* int64 index */
#define FSSH_S_ULONG_LONG_INDEX	00020000000	/* uint64 index */
#define FSSH_S_FLOAT_INDEX		00040000000	/* float index */
#define FSSH_S_DOUBLE_INDEX		00001000000	/* double index */
#define FSSH_S_ALLOW_DUPS		00002000000	/* allow duplicate entries (currently unused) */

/* link types */
#define	FSSH_S_LINK_SELF_HEALING	00001000000	/* link will be updated if you move its target */
#define FSSH_S_LINK_AUTO_DELETE	00002000000	/* link will be deleted if you delete its target */

/* standard file types */
#define FSSH_S_IFMT				00000170000 /* type of file */
#define	FSSH_S_IFLNK			00000120000 /* symbolic link */
#define FSSH_S_IFREG 			00000100000 /* regular */
#define FSSH_S_IFBLK 			00000060000 /* block special */
#define FSSH_S_IFDIR 			00000040000 /* directory */
#define FSSH_S_IFCHR 			00000020000 /* character special */
#define FSSH_S_IFIFO 			00000010000 /* fifo */

#define FSSH_S_ISREG(mode)		(((mode) & FSSH_S_IFMT) == FSSH_S_IFREG)
#define FSSH_S_ISLNK(mode)		(((mode) & FSSH_S_IFMT) == FSSH_S_IFLNK)
#define FSSH_S_ISBLK(mode)		(((mode) & FSSH_S_IFMT) == FSSH_S_IFBLK)
#define FSSH_S_ISDIR(mode)		(((mode) & FSSH_S_IFMT) == FSSH_S_IFDIR)
#define FSSH_S_ISCHR(mode)		(((mode) & FSSH_S_IFMT) == FSSH_S_IFCHR)
#define FSSH_S_ISFIFO(mode)		(((mode) & FSSH_S_IFMT) == FSSH_S_IFIFO)
#define FSSH_S_ISINDEX(mode)	(((mode) & FSSH_S_INDEX_DIR) == FSSH_S_INDEX_DIR)

#define	FSSH_S_IUMSK 07777		/* user settable bits */

#define FSSH_S_ISUID 04000       /* set user id on execution */
#define FSSH_S_ISGID 02000       /* set group id on execution */

#define FSSH_S_ISVTX 01000       /* save swapped text even after use (sticky bit) */

#define FSSH_S_IRWXU 00700       /* read, write, execute: owner */
#define FSSH_S_IRUSR 00400       /* read permission: owner */
#define FSSH_S_IWUSR 00200       /* write permission: owner */
#define FSSH_S_IXUSR 00100       /* execute permission: owner */
#define FSSH_S_IRWXG 00070       /* read, write, execute: group */
#define FSSH_S_IRGRP 00040       /* read permission: group */
#define FSSH_S_IWGRP 00020       /* write permission: group */
#define FSSH_S_IXGRP 00010       /* execute permission: group */
#define FSSH_S_IRWXO 00007       /* read, write, execute: other */
#define FSSH_S_IROTH 00004       /* read permission: other */
#define FSSH_S_IWOTH 00002       /* write permission: other */
#define FSSH_S_IXOTH 00001       /* execute permission: other */

#define FSSH_ACCESSPERMS	(FSSH_S_IRWXU | FSSH_S_IRWXG | FSSH_S_IRWXO)
#define FSSH_ALLPERMS		(FSSH_S_ISUID | FSSH_S_ISGID | FSSH_S_ISVTX \
								| FSSH_S_IRWXU | FSSH_S_IRWXG | FSSH_S_IRWXO)
#define	FSSH_DEFFILEMODE	(FSSH_S_IRUSR | FSSH_S_IWUSR | FSSH_S_IRGRP \
								| FSSH_S_IWGRP | FSSH_S_IROTH | FSSH_S_IWOTH)
	/* default file mode, everyone can read/write */

#ifdef __cplusplus
extern "C" {
#endif

extern int    		fssh_chmod(const char *path, fssh_mode_t mode);
extern int 	  		fssh_fchmod(int fd, fssh_mode_t mode);
extern int    		fssh_mkdir(const char *path, fssh_mode_t mode);
extern int    		fssh_mkfifo(const char *path, fssh_mode_t mode);
extern fssh_mode_t	fssh_umask(fssh_mode_t cmask);

extern int    		fssh_stat(const char *path, struct fssh_stat *st);
extern int    		fssh_fstat(int fd, struct fssh_stat *st);
extern int    		fssh_lstat(const char *path, struct fssh_stat *st);

#ifdef __cplusplus
}
#endif

#endif	/* _FSSH_SYS_STAT_H_ */
