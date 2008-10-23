/*
  This file contains some kit-wide typedefs and structs that basically
  emulate most of a normal posix-y type system.  The purpose of hiding
  everything behind these typedefs is to avoid inconsistencies between
  various systems (such as the difference in size between off_t on BeOS
  and some versions of Unix).  To further avoid complications I've also
  hidden the stat and dirent structs since those vary even more widely.

  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/


#ifndef _COMPAT_H
#define _COMPAT_H

#define _FS_INTERFACE_H
	// don't include that file

#if (!defined(__BEOS__) && !defined(__HAIKU__))
#	define dprintf build_platform_dprintf
#	include <stdio.h>
#	undef dprintf

typedef struct dirent dirent_t;
typedef struct iovec iovec;

#endif	// __BEOS__

#include <errno.h>
#ifndef _ERRORS_H
#	define _ERRORS_H
	// don't include <Errors.h>, we use the platform <errno.h>
#endif

#include <dirent.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#if (defined(__BEOS__) || defined(__HAIKU__))
#	include <OS.h>				/* for typedefs and prototypes */
#	include <image.h>			/* for a few typedefs */
#	include <Drivers.h>			/* for various ioctl structs, etc */
#	include <iovec.h>			/* because we're boneheads sometimes */
#elif HAIKU_HOST_PLATFORM_FREEBSD
/* BSD Compilant stat.h */
#	define __BSD_VISIBLE 1
#undef __XSI_VISIBLE
#	define __XSI_VISIBLE 1
	/* for mknod */
#	define __POSIX_VISIBLE 200112
	/* for S_ISLNK, S_ISSOCK and lstat */
#	include <sys/stat.h>
#	include <sys/uio.h>
#	include <image.h>			/* for a few typedefs */
#	include <Drivers.h>			/* for various ioctl structs, etc */
typedef unsigned long ulong;
#else
#	include <sys/uio.h>
#	include <image.h>			/* for a few typedefs */
#	include <Drivers.h>			/* for various ioctl structs, etc */
#endif

#include <fs_attr.h>

#include "errors.h"


/*
  By default (for portability reasons) the size of off_t's and ino_t's
  is 32-bit.  You can change the file system to be 64-bit if you want
  by defining OFF_T_SIZE to be 8.

  NOTE: if you change the size of OFF_T_SIZE to be 8 you will have to
        go through the code and change any calls to printf() to use the
        appropriate format for 64-bit integers on your OS.  I have seen
        4 different formats now: %Ld (BeOS and Linux), %qd (FreeBSD),
        %lld (Irix) and %I64d (NT).
*/
#define OFF_T_SIZE 8

#if OFF_T_SIZE == 4
typedef long fs_off_t;
typedef long my_ino_t;
#elif OFF_T_SIZE == 8
typedef long long fs_off_t;
typedef long long my_ino_t;
#else
#error OFF_T_SIZE must be either 4 or 8.
#endif

typedef int my_dev_t;
typedef int my_mode_t;
typedef int my_uid_t;
typedef int my_gid_t;

/* This is the maximum length of a file name.  Adjust it as you see fit */
#define FILE_NAME_LENGTH    256

/* This is maximum name size for naming a volume or semaphore/lock */
#define IDENT_NAME_LENGTH   32 


typedef struct my_dirent {
    my_dev_t        d_dev;
    my_dev_t        d_pdev;
    my_ino_t        d_ino;
    my_ino_t        d_pino;
    unsigned short  d_reclen;
    char            d_name[1];
} my_dirent_t;

typedef struct {
    int                 fd;
    struct my_dirent    ent;
} MY_DIR;


/*
  This is a pretty regular stat structure but it's our "internal"
  version since if we depended on the host version we'd be exposed
  to all sorts of nasty things (different sized ino_t's, etc).
  We also can't use the normal naming style of "st_" for each field
  name because on some systems fields like st_atime are really just
  define's that expand to all sorts of weird stuff.
*/  
struct my_stat {
    my_dev_t        dev;        /* "device" that this file resides on */
    my_ino_t        ino;        /* this file's inode #, unique per device */
    my_mode_t       mode;       /* mode bits (rwx for user, group, etc) */
    int             nlink;      /* number of hard links to this file */
    my_uid_t        uid;        /* user id of the owner of this file */
    my_gid_t        gid;        /* group id of the owner of this file */
    fs_off_t        size;       /* size in bytes of this file */
	dev_t			rdev;		/* device type (not used) */
    size_t          blksize;    /* preferred block size for i/o */
    time_t          atime;      /* last access time */
    time_t          mtime;      /* last modification time */
    time_t          ctime;      /* last change time, not creation time */
    time_t          crtime;     /* creation time; not posix but useful */
};


#define MY_S_ATTR_DIR          01000000000 /* attribute directory */
#define MY_S_ATTR              02000000000 /* attribute */
#define MY_S_INDEX_DIR         04000000000 /* index (or index directory) */
#define MY_S_STR_INDEX         00100000000 /* string index */
#define MY_S_INT_INDEX         00200000000 /* int32 index */
#define MY_S_UINT_INDEX        00400000000 /* uint32 index */
#define MY_S_LONG_LONG_INDEX   00010000000 /* int64 index */
#define MY_S_ULONG_LONG_INDEX  00020000000 /* uint64 index */
#define MY_S_FLOAT_INDEX       00040000000 /* float index */
#define MY_S_DOUBLE_INDEX      00001000000 /* double index */
#define MY_S_ALLOW_DUPS        00002000000 /* allow duplicate entries (currently unused) */

#define MY_S_LINK_SELF_HEALING 00001000000 /* link will be updated if you move its target */
#define MY_S_LINK_AUTO_DELETE  00002000000 /* link will be deleted if you delete its target */

#define     MY_S_IFMT        00000170000 /* type of file */
#define     MY_S_IFLNK       00000120000 /* symbolic link */
#define     MY_S_IFREG       00000100000 /* regular */
#define     MY_S_IFBLK       00000060000 /* block special */
#define     MY_S_IFDIR       00000040000 /* directory */
#define     MY_S_IFCHR       00000020000 /* character special */
#define     MY_S_IFIFO       00000010000 /* fifo */

#define     MY_S_ISREG(m)    (((m) & MY_S_IFMT) == MY_S_IFREG)
#define     MY_S_ISLNK(m)    (((m) & MY_S_IFMT) == MY_S_IFLNK)
#define     MY_S_ISBLK(m)    (((m) & MY_S_IFMT) == MY_S_IFBLK)
#define     MY_S_ISDIR(m)    (((m) & MY_S_IFMT) == MY_S_IFDIR)
#define     MY_S_ISCHR(m)    (((m) & MY_S_IFMT) == MY_S_IFCHR)
#define     MY_S_ISFIFO(m)   (((m) & MY_S_IFMT) == MY_S_IFIFO)

#define MY_S_IUMSK 07777     /* user settable bits */

#define MY_S_ISUID 04000     /* set user id on execution */
#define MY_S_ISGID 02000     /* set group id on execution */

#define MY_S_ISVTX 01000     /* save swapped text even after use */

#define MY_S_IRWXU 00700     /* read, write, execute: owner */
#define MY_S_IRUSR 00400     /* read permission: owner */
#define MY_S_IWUSR 00200     /* write permission: owner */
#define MY_S_IXUSR 00100     /* execute permission: owner */
#define MY_S_IRWXG 00070     /* read, write, execute: group */
#define MY_S_IRGRP 00040     /* read permission: group */
#define MY_S_IWGRP 00020     /* write permission: group */
#define MY_S_IXGRP 00010     /* execute permission: group */
#define MY_S_IRWXO 00007     /* read, write, execute: other */
#define MY_S_IROTH 00004     /* read permission: other */
#define MY_S_IWOTH 00002     /* write permission: other */
#define MY_S_IXOTH 00001     /* execute permission: other */

#define MY_O_RDONLY        0   /* read only */
#define MY_O_WRONLY        1   /* write only */
#define MY_O_RDWR          2   /* read and write */
#define MY_O_RWMASK        3   /* Mask to get open mode */

#define MY_O_CLOEXEC       0x0040  /* close fd on exec */
#define MY_O_NONBLOCK      0x0080  /* non blocking io */
#define MY_O_EXCL          0x0100  /* exclusive creat */
#define MY_O_CREAT         0x0200  /* create and open file */
#define MY_O_TRUNC         0x0400  /* open with truncation */
#define MY_O_APPEND        0x0800  /* to end of file */
#define MY_O_NOCTTY        0x1000  /* currently unsupported */
#define MY_O_NOTRAVERSE    0x2000  /* do not traverse leaf link */
#define MY_O_ACCMODE       0x0003  /* currently unsupported */
#define MY_O_TEXT          0x4000  /* CR-LF translation    */
#define MY_O_BINARY        0x8000  /* no translation   */

#define MY_SEEK_SET 0
#define MY_SEEK_CUR 1
#define MY_SEEK_END 2

// O_NOTRAVERSE is called O_NOFOLLOW under Linux
#ifndef O_NOTRAVERSE
	#ifdef O_NOFOLLOW
		#define O_NOTRAVERSE O_NOFOLLOW
	#else
		#define O_NOTRAVERSE 0
	#endif
#endif
#ifndef S_IUMSK
	#define S_IUMSK (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO)
#endif


#if (defined(__BEOS__) || defined(__HAIKU__))

typedef attr_info my_attr_info;

#else	// ! __BEOS__

typedef struct my_attr_info
{
	uint32		type;
	fs_off_t	size;
} my_attr_info;

#endif	// ! __BEOS__


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if (!defined(__BEOS__) && !defined(__HAIKU__))

#ifdef __cplusplus
extern "C" {
#endif

ssize_t    read_pos(int fd, fs_off_t _pos, void *data,  size_t nbytes);
ssize_t    write_pos(int fd, fs_off_t _pos, const void *data,  size_t nbytes);
ssize_t    readv_pos(int fd, fs_off_t _pos, const struct iovec *iov, int count);
ssize_t    writev_pos(int fd, fs_off_t _pos, const struct iovec *iov,  int count);

#ifdef __cplusplus
}
#endif

#endif /* ! __BEOS__ */

#ifdef __cplusplus
extern "C" {
#endif

void     panic(const char *msg, ...);
int      device_is_read_only(const char *device);
int      get_device_block_size(int fd);
fs_off_t get_num_device_blocks(int fd);
int      device_is_removeable(int fd);
int      lock_removeable_device(int fd, bool on_or_off);
void     hexdump(void *address, int size);

#ifdef __cplusplus
}
#endif

#endif /* _COMPAT_H */
