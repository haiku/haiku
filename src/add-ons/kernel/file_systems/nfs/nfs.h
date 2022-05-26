#ifndef _NFS_H

#define _NFS_H

#include <SupportDefs.h>

#define NFS_VERSION 2
#define NFS_PROGRAM 100003

typedef enum nfs_stat 
{ 
	NFS_OK = 0, 
	NFSERR_PERM=1, 
	NFSERR_NOENT=2, 
	NFSERR_IO=5, 
	NFSERR_NXIO=6, 
	NFSERR_ACCES=13, 
	NFSERR_EXIST=17, 
	NFSERR_NODEV=19, 
	NFSERR_NOTDIR=20, 
	NFSERR_ISDIR=21, 
	NFSERR_FBIG=27, 
	NFSERR_NOSPC=28, 
	NFSERR_ROFS=30, 
	NFSERR_NAMETOOLONG=63, 
	NFSERR_NOTEMPTY=66, 
	NFSERR_DQUOT=69, 
	NFSERR_STALE=70, 
	NFSERR_WFLUSH=99 
} nfs_stat; 
       
       
typedef enum nfs_ftype 
{ 
	NFS_NFNON = 0, 
	NFS_NFREG = 1, 
	NFS_NFDIR = 2, 
	NFS_NFBLK = 3, 
	NFS_NFCHR = 4, 
	NFS_NFLNK = 5 
} nfs_ftype; 
          
//#define NFS_MAXDATA 8192 
#define NFS_MAXDATA 1024

/* The maximum number of bytes in a pathname argument. */ 
#define NFS_MAXPATHLEN 1024 

/* The maximum number of bytes in a file name argument. */ 
#define NFS_MAXNAMLEN 255

/* The size in bytes of the opaque "cookie" passed by READDIR. */ 
#define NFS_COOKIESIZE 4 

/* The size in bytes of the opaque file handle. */ 
#define NFS_FHSIZE 32

enum
{
	NFSPROC_NULL       = 0, 
	NFSPROC_GETATTR    = 1, 
	NFSPROC_SETATTR    = 2, 
	NFSPROC_ROOT       = 3, 
	NFSPROC_LOOKUP     = 4, 
	NFSPROC_READLINK   = 5, 
	NFSPROC_READ       = 6, 
	NFSPROC_WRITECACHE = 7, 
	NFSPROC_WRITE      = 8, 
	NFSPROC_CREATE     = 9, 
	NFSPROC_REMOVE     = 10, 
	NFSPROC_RENAME     = 11, 
	NFSPROC_LINK       = 12, 
	NFSPROC_SYMLINK    = 13, 
	NFSPROC_MKDIR      = 14, 
	NFSPROC_RMDIR      = 15, 
	NFSPROC_READDIR    = 16, 
	NFSPROC_STATFS     = 17
};

struct nfs_fhandle
{
	char opaque[NFS_FHSIZE];
};

struct nfs_cookie
{
	char opaque[NFS_COOKIESIZE];
};

#endif
