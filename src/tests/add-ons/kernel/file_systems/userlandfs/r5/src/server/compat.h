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


#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#if (defined(__BEOS__) || defined(__HAIKU__))
#include <OS.h>              /* for typedefs and prototypes */
#include <image.h>           /* for a few typedefs */
#include <Drivers.h>         /* for various ioctl structs, etc */
#include <iovec.h>           /* because we're boneheads sometimes */
#else
#include <sys/uio.h>
#endif


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
    my_ino_t        d_ino;
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
    size_t          blksize;    /* preferred block size for i/o */
    time_t          atime;      /* last access time */
    time_t          mtime;      /* last modification time */
    time_t          ctime;      /* last change time, not creation time */
    time_t          crtime;     /* creation time; not posix but useful */
};


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



#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if (!defined(__BEOS__) && !defined(__HAIKU__))
typedef long               sem_id;
typedef unsigned char      uchar;
typedef short              int16;
typedef unsigned short     uint16;
typedef int                int32;
typedef unsigned int       uint32;
#define ulong unsigned long         /* make it a #define to avoid conflicts */
typedef long long          int64;
typedef unsigned long long uint64;
typedef unsigned int       port_id;
typedef int                bool;
typedef int                image_id;
typedef long long          bigtime_t;
typedef long               thread_id;
typedef long               status_t;

sem_id     create_sem(long count, const char *name);
long       delete_sem(sem_id sem);
long       acquire_sem(sem_id sem);
long       acquire_sem_etc(sem_id sem, int count, int flags,
                           bigtime_t microsecond_timeout);
long       release_sem(sem_id sem);
long       release_sem_etc(sem_id sem, long count, long flags);

long       atomic_add(long *value, long addvalue);
int        snooze(bigtime_t f);
bigtime_t  system_time(void);
ssize_t    read_pos(int fd, fs_off_t _pos, void *data,  size_t nbytes);
ssize_t    write_pos(int fd, fs_off_t _pos, const void *data,  size_t nbytes);
ssize_t    readv_pos(int fd, fs_off_t _pos, struct iovec *iov, int count);
ssize_t    writev_pos(int fd, fs_off_t _pos, struct iovec *iov,  int count);


#endif /* __BEOS__ */

void     panic(const char *msg, ...);
int      device_is_read_only(const char *device);
int      get_device_block_size(int fd);
fs_off_t get_num_device_blocks(int fd);
int      device_is_removeable(int fd);
int      lock_removeable_device(int fd, bool on_or_off);
void     hexdump(void *address, int size);


#endif /* _COMPAT_H */
