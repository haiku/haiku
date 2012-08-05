/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef NFS4DEFS_H
#define NFS4DEFS_H


#include <fcntl.h>
#include <sys/stat.h>

#include <SupportDefs.h>


enum Procedure {
	ProcNull		= 0,
	ProcCompound	= 1
};

enum CallbackProcedure {
	CallbackProcNull		= 0,
	CallbackProcCompound	= 1
};

enum Opcode {
	OpAccess				= 3,
	OpClose					= 4,
	OpCommit				= 5,
	OpCreate				= 6,
	OpGetAttr				= 9,
	OpGetFH					= 10,
	OpLink					= 11,
	OpLock					= 12,
	OpLockT					= 13,
	OpLockU					= 14,
	OpLookUp				= 15,
	OpLookUpUp				= 16,
	OpNverify				= 17,
	OpOpen					= 18,
	OpOpenConfirm			= 20,
	OpPutFH					= 22,
	OpPutRootFH				= 24,
	OpRead					= 25,
	OpReadDir				= 26,
	OpReadLink				= 27,
	OpRemove				= 28,
	OpRename				= 29,
	OpRenew					= 30,
	OpSaveFH				= 32,
	OpSetAttr				= 34,
	OpSetClientID			= 35,
	OpSetClientIDConfirm	= 36,
	OpVerify				= 37,
	OpWrite					= 38,
	OpReleaseLockOwner		= 39
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


static inline bool sIsAttrSet(Attribute attr, const uint32* bitmap,
	uint32 count)
{
	if ((uint32)attr / 32 >= count)
		return false;

	return (bitmap[attr / 32] & 1 << attr % 32) != 0;
}


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

enum OpenAccess {
	OPEN4_SHARE_ACCESS_READ		= 1,
	OPEN4_SHARE_ACCESS_WRITE	= 2,
	OPEN4_SHARE_ACCESS_BOTH		= 3
};


static inline OpenAccess
sModeToAccess(int mode)
{
	switch (mode & O_RWMASK) {
		case O_RDONLY:
			return OPEN4_SHARE_ACCESS_READ;
		case O_WRONLY:
			return OPEN4_SHARE_ACCESS_WRITE;
		case O_RDWR:
			return OPEN4_SHARE_ACCESS_BOTH;
	}

	return OPEN4_SHARE_ACCESS_READ;
}


enum OpenCreate {
	OPEN4_NOCREATE			= 0,
	OPEN4_CREATE			= 1
};

enum OpenCreateHow {
	UNCHECKED4				= 0,
	GUARDED4				= 1,
	EXCLUSIVE4				= 2
};

enum OpenClaim {
	CLAIM_NULL				= 0,
	CLAIM_PREVIOUS			= 1,
	CLAIM_DELEGATE_CUR		= 2,
	CLAIM_DELEGATE_PREV		= 3
};

enum OpenDelegation {
	OPEN_DELEGATE_NONE		= 0,
	OPEN_DELEGATE_READ		= 1,
	OPEN_DELEGATE_WRITE		= 2
};

struct OpenDelegationData {
	OpenDelegation	fType;

	uint32			fStateSeq;
	uint32			fStateID[3];

	bool			fRecall;
	uint64			fSpaceLimit;
};

enum OpenFlags {
	OPEN4_RESULT_CONFIRM		= 2,
	OPEN4_RESULT_LOCKTYPE_POSIX	= 4
};

enum {
	NFS_LIMIT_SIZE		= 1,
	NFS_LIMIT_BLOCKS	= 2
};

struct ChangeInfo {
	bool	fAtomic;
	uint64	fBefore;
	uint64	fAfter;
};

enum WriteStable {
	UNSTABLE4				= 0,
	DATA_SYNC4				= 1,
	FILE_SYNC4				= 2
};

enum LockType {
	READ_LT			= 1,
	WRITE_LT		= 2,
	READW_LT		= 3,
	WRITEW_LT		= 4
};


static inline LockType
sGetLockType(short type, bool wait) {
	switch (type) {
		case F_RDLCK:	return wait ? READW_LT : READ_LT;
		case F_WRLCK:	return wait ? WRITEW_LT : WRITE_LT;
		default:		return READ_LT;
	}
}


static inline short 
sLockTypeToHaiku(LockType type) {
	switch (type) {
		case READ_LT:
		case READW_LT:
						return F_RDLCK;

		case WRITE_LT:
		case WRITEW_LT:
						return F_WRLCK;

		default:		return F_UNLCK;
	}
}


enum Errors {
	NFS4_OK						= 0,
	NFS4ERR_PERM				= 1,
	NFS4ERR_NOENT				= 2,
	NFS4ERR_IO					= 5,
	NFS4ERR_NXIO				= 6,
	NFS4ERR_ACCESS				= 13,
	NFS4ERR_EXIST				= 17,
	NFS4ERR_XDEV				= 18,
	NFS4ERR_NOTDIR				= 20,
	NFS4ERR_ISDIR				= 21,
	NFS4ERR_INVAL				= 22,
	NFS4ERR_FBIG				= 27,
	NFS4ERR_BADHANDLE			= 10001,
	NFS4ERR_BAD_COOKIE			= 10003,
	NFS4ERR_NOTSUPP				= 10004,
	NFS4ERR_TOOSMALL			= 10005,
	NFS4ERR_SERVERFAULT			= 10006,
	NFS4ERR_BADTYPE				= 10007,
	NFS4ERR_DELAY				= 10008,
	NFS4ERR_SAME				= 10009,
	NFS4ERR_DENIED				= 10010,
	NFS4ERR_EXPIRED				= 10011,
	NFS4ERR_LOCKED				= 10012,
	NFS4ERR_GRACE				= 10013,
	NFS4ERR_FHEXPIRED			= 10014,
	NFS4ERR_SHARE_DENIED		= 10015,
	NFS4ERR_WRONGSEC			= 10016,
	NFS4ERR_CLID_INUSE			= 10017,
	NFS4ERR_RESOURCE			= 10018,
	NFS4ERR_MOVED				= 10019,
	NFS4ERR_NOFILEHANDLE		= 10020,
	NFS4ERR_MINOR_VERS_MISMATCH	= 10021,
	NFS4ERR_STALE_CLIENTID		= 10022,
	NFS4ERR_STALE_STATEID		= 10023,
	NFS4ERR_OLD_STATEID			= 10024,
	NFS4ERR_BAD_STATEID			= 10025,
	NFS4ERR_BAD_SEQID			= 10026,
	NFS4ERR_NOT_SAME			= 10027,
	NFS4ERR_LOCK_RANGE			= 10028,
	NFS4ERR_SYMLINK				= 10029,
	NFS4ERR_RESTOREFH			= 10030,
	NFS4ERR_LEASE_MOVED			= 10031,
	NFS4ERR_ATTRNOTSUPP			= 10032,
	NFS4ERR_NO_GRACE			= 10033,
	NFS4ERR_RECLAIM_BAD			= 10034,
	NFS4ERR_RECLAIM_CONFLICT	= 10035,
	NFS4ERR_BADXDR				= 10036,
	NFS4ERR_LOCKS_HELD			= 10037,
	NFS4ERR_OPENMODE			= 10038,
	NFS4ERR_BADOWNER			= 10039,
	NFS4ERR_BADCHAR				= 10040,
	NFS4ERR_BADNAME				= 10041,
	NFS4ERR_BAD_RANGE			= 10042,
	NFS4ERR_LOCK_NOTSUPP		= 10043,
	NFS4ERR_OP_ILLEGAL			= 10044,
	NFS4ERR_DEADLOCK			= 10045,
	NFS4ERR_FILE_OPEN			= 10046,
	NFS4ERR_ADMIN_REVOKED		= 10047,
	NFS4ERR_CB_PATH_DOWN		= 10048
};


static inline bigtime_t
sSecToBigTime(uint32 sec)
{
	return static_cast<bigtime_t>(sec) * 1000000;
}


#endif	// NFS4DEFS_H

