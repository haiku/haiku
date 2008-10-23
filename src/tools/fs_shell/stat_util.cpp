/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "stat_util.h"

#include <fcntl.h>
#include <sys/stat.h>

#include "fssh_fcntl.h"
#include "fssh_stat.h"


namespace FSShell {

// Haiku only mode_t flags
#ifndef HAIKU_HOST_PLATFORM_HAIKU
#	define S_LINK_SELF_HEALING	0
#	define S_LINK_AUTO_DELETE	0
#endif


fssh_mode_t
from_platform_mode(mode_t mode)
{
	#define SET_ST_MODE_BIT(flag, fsshFlag)	\
		if (mode & flag)			\
			fsshMode |= fsshFlag;

	fssh_mode_t fsshMode = 0;

	// BeOS/Haiku only
	#if (defined(__BEOS__) || defined(__HAIKU__))
		SET_ST_MODE_BIT(FSSH_S_ATTR_DIR, S_ATTR_DIR);
		SET_ST_MODE_BIT(FSSH_S_ATTR, S_ATTR);
		SET_ST_MODE_BIT(FSSH_S_INDEX_DIR, S_INDEX_DIR);
		SET_ST_MODE_BIT(FSSH_S_INT_INDEX, S_INT_INDEX);
		SET_ST_MODE_BIT(FSSH_S_UINT_INDEX, S_UINT_INDEX);
		SET_ST_MODE_BIT(FSSH_S_LONG_LONG_INDEX, S_LONG_LONG_INDEX);
		SET_ST_MODE_BIT(FSSH_S_ULONG_LONG_INDEX, S_ULONG_LONG_INDEX);
		SET_ST_MODE_BIT(FSSH_S_FLOAT_INDEX, S_FLOAT_INDEX);
		SET_ST_MODE_BIT(FSSH_S_DOUBLE_INDEX, S_DOUBLE_INDEX);
		SET_ST_MODE_BIT(FSSH_S_ALLOW_DUPS, S_ALLOW_DUPS);
		SET_ST_MODE_BIT(FSSH_S_LINK_SELF_HEALING, S_LINK_SELF_HEALING);
		SET_ST_MODE_BIT(FSSH_S_LINK_AUTO_DELETE, S_LINK_AUTO_DELETE);
	#endif

	switch (mode & S_IFMT) {
		case S_IFLNK:
			fsshMode |= FSSH_S_IFLNK;
			break;
		case S_IFREG:
			fsshMode |= FSSH_S_IFREG;
			break;
		case S_IFBLK:
			fsshMode |= FSSH_S_IFBLK;
			break;
		case S_IFDIR:
			fsshMode |= FSSH_S_IFDIR;
			break;
		case S_IFIFO:
			fsshMode |= FSSH_S_IFIFO;
			break;
	}

	SET_ST_MODE_BIT(FSSH_S_ISUID, S_ISUID);
	SET_ST_MODE_BIT(FSSH_S_ISGID, S_ISGID);
	SET_ST_MODE_BIT(FSSH_S_ISVTX, S_ISVTX);
	SET_ST_MODE_BIT(FSSH_S_IRUSR, S_IRUSR);
	SET_ST_MODE_BIT(FSSH_S_IWUSR, S_IWUSR);
	SET_ST_MODE_BIT(FSSH_S_IXUSR, S_IXUSR);
	SET_ST_MODE_BIT(FSSH_S_IRGRP, S_IRGRP);
	SET_ST_MODE_BIT(FSSH_S_IWGRP, S_IWGRP);
	SET_ST_MODE_BIT(FSSH_S_IXGRP, S_IXGRP);
	SET_ST_MODE_BIT(FSSH_S_IROTH, S_IROTH);
	SET_ST_MODE_BIT(FSSH_S_IWOTH, S_IWOTH);
	SET_ST_MODE_BIT(FSSH_S_IXOTH, S_IXOTH);

	#undef SET_ST_MODE_BIT

	return fsshMode;
}


mode_t
to_platform_mode(fssh_mode_t fsshMode)
{
	#define SET_ST_MODE_BIT(flag, fsshFlag)	\
		if (fsshMode & fsshFlag)				\
			mode |= flag;

	mode_t mode = 0;

	// BeOS/Haiku only
	#if (defined(__BEOS__) || defined(__HAIKU__))
		SET_ST_MODE_BIT(FSSH_S_ATTR_DIR, S_ATTR_DIR);
		SET_ST_MODE_BIT(FSSH_S_ATTR, S_ATTR);
		SET_ST_MODE_BIT(FSSH_S_INDEX_DIR, S_INDEX_DIR);
		SET_ST_MODE_BIT(FSSH_S_INT_INDEX, S_INT_INDEX);
		SET_ST_MODE_BIT(FSSH_S_UINT_INDEX, S_UINT_INDEX);
		SET_ST_MODE_BIT(FSSH_S_LONG_LONG_INDEX, S_LONG_LONG_INDEX);
		SET_ST_MODE_BIT(FSSH_S_ULONG_LONG_INDEX, S_ULONG_LONG_INDEX);
		SET_ST_MODE_BIT(FSSH_S_FLOAT_INDEX, S_FLOAT_INDEX);
		SET_ST_MODE_BIT(FSSH_S_DOUBLE_INDEX, S_DOUBLE_INDEX);
		SET_ST_MODE_BIT(FSSH_S_ALLOW_DUPS, S_ALLOW_DUPS);
		SET_ST_MODE_BIT(FSSH_S_LINK_SELF_HEALING, S_LINK_SELF_HEALING);
		SET_ST_MODE_BIT(FSSH_S_LINK_AUTO_DELETE, S_LINK_AUTO_DELETE);
	#endif

	switch (fsshMode & FSSH_S_IFMT) {
		case FSSH_S_IFLNK:
			mode |= S_IFLNK;
			break;
		case FSSH_S_IFREG:
			mode |= S_IFREG;
			break;
		case FSSH_S_IFBLK:
			mode |= S_IFBLK;
			break;
		case FSSH_S_IFDIR:
			mode |= S_IFDIR;
			break;
		case FSSH_S_IFIFO:
			mode |= S_IFIFO;
			break;
	}

	SET_ST_MODE_BIT(FSSH_S_ISUID, S_ISUID);
	SET_ST_MODE_BIT(FSSH_S_ISGID, S_ISGID);
	SET_ST_MODE_BIT(FSSH_S_ISVTX, S_ISVTX);
	SET_ST_MODE_BIT(FSSH_S_IRUSR, S_IRUSR);
	SET_ST_MODE_BIT(FSSH_S_IWUSR, S_IWUSR);
	SET_ST_MODE_BIT(FSSH_S_IXUSR, S_IXUSR);
	SET_ST_MODE_BIT(FSSH_S_IRGRP, S_IRGRP);
	SET_ST_MODE_BIT(FSSH_S_IWGRP, S_IWGRP);
	SET_ST_MODE_BIT(FSSH_S_IXGRP, S_IXGRP);
	SET_ST_MODE_BIT(FSSH_S_IROTH, S_IROTH);
	SET_ST_MODE_BIT(FSSH_S_IWOTH, S_IWOTH);
	SET_ST_MODE_BIT(FSSH_S_IXOTH, S_IXOTH);

	#undef SET_ST_MODE_BIT

	return mode;
}


void
from_platform_stat(const struct stat *st, struct fssh_stat *fsshStat)
{
	fsshStat->fssh_st_dev = st->st_dev;
	fsshStat->fssh_st_ino = st->st_ino;
	fsshStat->fssh_st_mode = from_platform_mode(st->st_mode);
	fsshStat->fssh_st_nlink = st->st_nlink;
	fsshStat->fssh_st_uid = st->st_uid;
	fsshStat->fssh_st_gid = st->st_gid;
	fsshStat->fssh_st_size = st->st_size;
	fsshStat->fssh_st_blksize = st->st_blksize;
	fsshStat->fssh_st_atime = st->st_atime;
	fsshStat->fssh_st_mtime = st->st_mtime;
	fsshStat->fssh_st_ctime = st->st_ctime;
	fsshStat->fssh_st_crtime = st->st_ctime;
	fsshStat->fssh_st_type = 0;
}


void
to_platform_stat(const struct fssh_stat *fsshStat, struct stat *st)
{
	st->st_dev = fsshStat->fssh_st_dev;
	st->st_ino = fsshStat->fssh_st_ino;
	st->st_mode = to_platform_mode(fsshStat->fssh_st_mode);
	st->st_nlink = fsshStat->fssh_st_nlink;
	st->st_uid = fsshStat->fssh_st_uid;
	st->st_gid = fsshStat->fssh_st_gid;
	st->st_size = fsshStat->fssh_st_size;
	st->st_blksize = fsshStat->fssh_st_blksize;
	st->st_atime = fsshStat->fssh_st_atime;
	st->st_mtime = fsshStat->fssh_st_mtime;
	st->st_ctime = fsshStat->fssh_st_ctime;
//	st->st_crtime = fsshStat->fssh_st_crtime;
//	st->st_type = fsshStat->fssh_st_type;
}


int
to_platform_open_mode(int fsshMode)
{
	#define SET_OPEN_MODE_FLAG(flag, fsshFlag)	\
		if (fsshMode & fsshFlag)				\
			mode |= flag;

	int mode = 0;

	// the r/w mode
	switch (fsshMode & FSSH_O_RWMASK) {
		case FSSH_O_RDONLY:
			mode |= O_RDONLY;
			break;
		case FSSH_O_WRONLY:
			mode |= O_WRONLY;
			break;
		case FSSH_O_RDWR:
			mode |= O_RDWR;
			break;
	}

	// the flags
	//SET_OPEN_MODE_FLAG(O_CLOEXEC, FSSH_O_CLOEXEC)
	SET_OPEN_MODE_FLAG(O_NONBLOCK, FSSH_O_NONBLOCK)
	SET_OPEN_MODE_FLAG(O_EXCL, FSSH_O_EXCL)
	SET_OPEN_MODE_FLAG(O_CREAT, FSSH_O_CREAT)
	SET_OPEN_MODE_FLAG(O_TRUNC, FSSH_O_TRUNC)
	SET_OPEN_MODE_FLAG(O_APPEND, FSSH_O_APPEND)
	SET_OPEN_MODE_FLAG(O_NOCTTY, FSSH_O_NOCTTY)
	SET_OPEN_MODE_FLAG(O_NOTRAVERSE, FSSH_O_NOTRAVERSE)
	//SET_OPEN_MODE_FLAG(O_TEXT, FSSH_O_TEXT)
	//SET_OPEN_MODE_FLAG(O_BINARY, FSSH_O_BINARY)

	#undef SET_OPEN_MODE_FLAG

	return mode;
}

}	// namespace FSShell
