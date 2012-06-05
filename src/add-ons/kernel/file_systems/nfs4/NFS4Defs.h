/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef NFS4DEFS_H
#define NFS4DEFS_H


#include <sys/stat.h>

#include <SupportDefs.h>

#include "Filehandle.h"

enum Procedure {
	ProcNull		= 0,
	ProcCompound	= 1
};

enum Opcode {
	OpAccess		= 3,
	OpGetAttr		= 9,
	OpGetFH			= 10,
	OpLookUp		= 15,
	OpLookUpUp		= 16,
	OpPutFH			= 22,
	OpPutRootFH		= 24,
	OpReadDir		= 26
};

enum Access {
	ACCESS4_READ		= 0x00000001,
	ACCESS4_LOOKUP		= 0x00000002,
	ACCESS4_MODIFY		= 0x00000004,
	ACCESS4_EXTEND		= 0x00000008,
	ACCESS4_DELETE		= 0x00000010,
	ACCESS4_EXECUTE		= 0x00000020
};

enum Attribute {
	// Mandatory Attributes
	FATTR4_SUPPORTED_ATTRS		= 0,
	FATTR4_TYPE					= 1,
	FATTR4_FH_EXPIRE_TYPE		= 2,
	FATTR4_CHANGE				= 3,
	FATTR4_SIZE					= 4,
	FATTR4_LINK_SUPPORT			= 5,
	FATTR4_SYMLINK_SUPPORT		= 6,
	FATTR4_NAMED_ATTR			= 7,
	FATTR4_FSID					= 8,
	FATTR4_UNIQUE_HANDLES		= 9,
	FATTR4_LEASE_TIME			= 10,
	FATTR4_RDATTR_ERROR			= 11,
	FATTR4_FILEHANDLE			= 19,

	// Recommended Attributes
	FATTR4_ACL					= 12,
	FATTR4_ACLSUPPORT			= 13,
	FATTR4_ARCHIVE				= 14,
	FATTR4_CANSETTIME			= 15,
	FATTR4_CASE_INSENSITIVE		= 16,
	FATTR4_CASE_PRESERVING		= 17,
	FATTR4_CHOWN_RESTRICTED		= 18,
	FATTR4_FILEID				= 20,
	FATTR4_FILES_AVAIL			= 21,
	FATTR4_FILES_FREE			= 22,
	FATTR4_FILES_TOTAL			= 23,
	FATTR4_FS_LOCATIONS			= 24,
	FATTR4_HIDDEN				= 25,
	FATTR4_HOMOGENEOUS			= 26,
	FATTR4_MAXFILESIZE			= 27,
	FATTR4_MAXLINK				= 28,
	FATTR4_MAXNAME				= 29,
	FATTR4_MAXREAD				= 30,
	FATTR4_MAXWRITE				= 31,
	FATTR4_MIMETYPE				= 32,
	FATTR4_MODE					= 33,
	FATTR4_NO_TRUNC				= 34,
	FATTR4_NUMLINKS				= 35,
	FATTR4_OWNER				= 36,
	FATTR4_OWNER_GROUP			= 37,
	FATTR4_QUOTA_AVAIL_HARD		= 38,
	FATTR4_QUOTA_AVAIL_SOFT		= 39,
	FATTR4_QUOTA_USED			= 40,
	FATTR4_RAWDEV				= 41,
	FATTR4_SPACE_AVAIL			= 42,
	FATTR4_SPACE_FREE			= 43,
	FATTR4_SPACE_TOTAL			= 44,
	FATTR4_SPACE_USED			= 45,
	FATTR4_SYSTEM				= 46,
	FATTR4_TIME_ACCESS			= 47,
	FATTR4_TIME_ACCESS_SET		= 48,
	FATTR4_TIME_BACKUP			= 49,
	FATTR4_TIME_CREATE			= 50,
	FATTR4_TIME_DELTA			= 51,
	FATTR4_TIME_METADATA		= 52,
	FATTR4_TIME_MODIFY			= 53,
	FATTR4_TIME_MODIFY_SET		= 54,
	FATTR4_MOUNTED_ON_FILEID	= 55,
	FATTR4_MAXIMUM_ATTR_ID
};

enum FileType {
	NF4REG			= 1,    /* Regular File */
	NF4DIR			= 2,    /* Directory */
	NF4BLK			= 3,    /* Special File - block device */
	NF4CHR			= 4,    /* Special File - character device */
	NF4LNK			= 5,    /* Symbolic Link */
	NF4SOCK			= 6,    /* Special File - socket */
	NF4FIFO			= 7,    /* Special File - fifo */
	NF4ATTRDIR		= 8,    /* Attribute Directory */
	NF4NAMEDATTR	= 9     /* Named Attribute */
};

static const mode_t sNFSFileTypeToHaiku[] = {
	S_IFREG, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK, S_IFSOCK, S_IFIFO,
	S_IFDIR, S_IFREG
};

enum FileHandleExpiryType {
	FH4_PERSISTENT			= 0x00,
	FH4_NOEXPIRE_WITH_OPEN	= 0x01,
	FH4_VOLATILE_ANY		= 0x02,
	FH4_VOL_MIGRATION		= 0x04,
	FH4_VOL_RENAME			= 0x08
};

enum Errors {
	NFS4_OK			= 0,
	NFS4ERR_PERM	= 1,
	NFS4ERR_NOENT	= 2,
	NFS4ERR_IO		= 5,
	NFS4ERR_NXIO	= 6,
	NFS4ERR_ACCESS	= 13,
	NFS4ERR_EXIST	= 17,
	NFS4ERR_XDEV	= 18,
	NFS4ERR_NOTDIR	= 20,
	NFS4ERR_ISDIR	= 21,
	NFS4ERR_INVAL	= 22,
	NFS4ERR_FBIG	= 27
	//...
};


#endif	// NFS4DEFS_H

