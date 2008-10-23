/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "stat_util.h"

#if (defined(__BEOS__) || defined(__HAIKU__))

void
from_platform_stat(const struct stat *st, struct my_stat *myst)
{
	myst->dev = st->st_dev;
	myst->ino = st->st_ino;
	myst->nlink = st->st_nlink;
	myst->uid = st->st_uid;
	myst->gid = st->st_gid;
	myst->size = st->st_size;
	myst->blksize = st->st_blksize;
	myst->atime = st->st_atime;
	myst->mtime = st->st_mtime;
	myst->ctime = st->st_ctime;
	myst->crtime = st->st_ctime;
	myst->mode = st->st_mode;
}


void
to_platform_stat(const struct my_stat *myst, struct stat *st)
{
	st->st_dev = myst->dev;
	st->st_ino = myst->ino;
	st->st_nlink = myst->nlink;
	st->st_uid = myst->uid;
	st->st_gid = myst->gid;
	st->st_size = myst->size;
	st->st_blksize = myst->blksize;
	st->st_atime = myst->atime;
	st->st_mtime = myst->mtime;
	st->st_ctime = myst->ctime;
	st->st_crtime = myst->crtime;
	st->st_mode = myst->mode;
}


int
to_platform_open_mode(int myMode)
{
	return myMode;
}


#else // !__BEOS__

#ifndef S_ATTR_DIR
	#define S_ATTR_DIR 0
#endif
#ifndef S_ATTR
	#define S_ATTR 0
#endif
#ifndef S_INDEX_DIR
	#define S_INDEX_DIR 0
#endif
#ifndef S_INT_INDEX
	#define S_INT_INDEX 0
#endif
#ifndef S_UINT_INDEX
	#define S_UINT_INDEX 0
#endif
#ifndef S_LONG_LONG_INDEX
	#define S_LONG_LONG_INDEX 0
#endif
#ifndef S_ULONG_LONG_INDEX
	#define S_ULONG_LONG_INDEX 0
#endif
#ifndef S_FLOAT_INDEX
	#define S_FLOAT_INDEX 0
#endif
#ifndef S_DOUBLE_INDEX
	#define S_DOUBLE_INDEX 0
#endif
#ifndef S_ALLOW_DUPS
	#define S_ALLOW_DUPS 0
#endif
#ifndef S_LINK_SELF_HEALING
	#define S_LINK_SELF_HEALING 0
#endif
#ifndef S_LINK_AUTO_DELETE
	#define S_LINK_AUTO_DELETE 0
#endif


my_mode_t
from_platform_mode(mode_t mode)
{
	#define SET_ST_MODE_BIT(flag, myFlag)	\
		if (mode & flag)			\
			myMode |= myFlag;

	my_mode_t myMode = 0;

	SET_ST_MODE_BIT(MY_S_ATTR_DIR, S_ATTR_DIR);
	SET_ST_MODE_BIT(MY_S_ATTR, S_ATTR);
	SET_ST_MODE_BIT(MY_S_INDEX_DIR, S_INDEX_DIR);
	SET_ST_MODE_BIT(MY_S_INT_INDEX, S_INT_INDEX);
	SET_ST_MODE_BIT(MY_S_UINT_INDEX, S_UINT_INDEX);
	SET_ST_MODE_BIT(MY_S_LONG_LONG_INDEX, S_LONG_LONG_INDEX);
	SET_ST_MODE_BIT(MY_S_ULONG_LONG_INDEX, S_ULONG_LONG_INDEX);
	SET_ST_MODE_BIT(MY_S_FLOAT_INDEX, S_FLOAT_INDEX);
	SET_ST_MODE_BIT(MY_S_DOUBLE_INDEX, S_DOUBLE_INDEX);
	SET_ST_MODE_BIT(MY_S_ALLOW_DUPS, S_ALLOW_DUPS);
	SET_ST_MODE_BIT(MY_S_LINK_SELF_HEALING, S_LINK_SELF_HEALING);
	SET_ST_MODE_BIT(MY_S_LINK_AUTO_DELETE, S_LINK_AUTO_DELETE);

	switch (mode & S_IFMT) {
		case S_IFLNK:
			myMode |= MY_S_IFLNK;
			break;
		case S_IFREG:
			myMode |= MY_S_IFREG;
			break;
		case S_IFBLK:
			myMode |= MY_S_IFBLK;
			break;
		case S_IFDIR:
			myMode |= MY_S_IFDIR;
			break;
		case S_IFIFO:
			myMode |= MY_S_IFIFO;
			break;
	}

	SET_ST_MODE_BIT(MY_S_ISUID, S_ISUID);
	SET_ST_MODE_BIT(MY_S_ISGID, S_ISGID);
	SET_ST_MODE_BIT(MY_S_ISVTX, S_ISVTX);
	SET_ST_MODE_BIT(MY_S_IRUSR, S_IRUSR);
	SET_ST_MODE_BIT(MY_S_IWUSR, S_IWUSR);
	SET_ST_MODE_BIT(MY_S_IXUSR, S_IXUSR);
	SET_ST_MODE_BIT(MY_S_IRGRP, S_IRGRP);
	SET_ST_MODE_BIT(MY_S_IWGRP, S_IWGRP);
	SET_ST_MODE_BIT(MY_S_IXGRP, S_IXGRP);
	SET_ST_MODE_BIT(MY_S_IROTH, S_IROTH);
	SET_ST_MODE_BIT(MY_S_IWOTH, S_IWOTH);
	SET_ST_MODE_BIT(MY_S_IXOTH, S_IXOTH);

	#undef SET_ST_MODE_BIT

	return myMode;
}


mode_t
to_platform_mode(my_mode_t myMode)
{
	#define SET_ST_MODE_BIT(flag, myFlag)	\
		if (myMode & myFlag)			\
			mode |= flag;

	mode_t mode = 0;

	SET_ST_MODE_BIT(MY_S_ATTR_DIR, S_ATTR_DIR);
	SET_ST_MODE_BIT(MY_S_ATTR, S_ATTR);
	SET_ST_MODE_BIT(MY_S_INDEX_DIR, S_INDEX_DIR);
	SET_ST_MODE_BIT(MY_S_INT_INDEX, S_INT_INDEX);
	SET_ST_MODE_BIT(MY_S_UINT_INDEX, S_UINT_INDEX);
	SET_ST_MODE_BIT(MY_S_LONG_LONG_INDEX, S_LONG_LONG_INDEX);
	SET_ST_MODE_BIT(MY_S_ULONG_LONG_INDEX, S_ULONG_LONG_INDEX);
	SET_ST_MODE_BIT(MY_S_FLOAT_INDEX, S_FLOAT_INDEX);
	SET_ST_MODE_BIT(MY_S_DOUBLE_INDEX, S_DOUBLE_INDEX);
	SET_ST_MODE_BIT(MY_S_ALLOW_DUPS, S_ALLOW_DUPS);
	SET_ST_MODE_BIT(MY_S_LINK_SELF_HEALING, S_LINK_SELF_HEALING);
	SET_ST_MODE_BIT(MY_S_LINK_AUTO_DELETE, S_LINK_AUTO_DELETE);

	switch (myMode & MY_S_IFMT) {
		case MY_S_IFLNK:
			mode |= S_IFLNK;
			break;
		case MY_S_IFREG:
			mode |= S_IFREG;
			break;
		case MY_S_IFBLK:
			mode |= S_IFBLK;
			break;
		case MY_S_IFDIR:
			mode |= S_IFDIR;
			break;
		case MY_S_IFIFO:
			mode |= S_IFIFO;
			break;
	}

	SET_ST_MODE_BIT(MY_S_ISUID, S_ISUID);
	SET_ST_MODE_BIT(MY_S_ISGID, S_ISGID);
	SET_ST_MODE_BIT(MY_S_ISVTX, S_ISVTX);
	SET_ST_MODE_BIT(MY_S_IRUSR, S_IRUSR);
	SET_ST_MODE_BIT(MY_S_IWUSR, S_IWUSR);
	SET_ST_MODE_BIT(MY_S_IXUSR, S_IXUSR);
	SET_ST_MODE_BIT(MY_S_IRGRP, S_IRGRP);
	SET_ST_MODE_BIT(MY_S_IWGRP, S_IWGRP);
	SET_ST_MODE_BIT(MY_S_IXGRP, S_IXGRP);
	SET_ST_MODE_BIT(MY_S_IROTH, S_IROTH);
	SET_ST_MODE_BIT(MY_S_IWOTH, S_IWOTH);
	SET_ST_MODE_BIT(MY_S_IXOTH, S_IXOTH);

	#undef SET_ST_MODE_BIT

	return mode;
}


void
from_platform_stat(const struct stat *st, struct my_stat *myst)
{
	myst->dev = st->st_dev;
	myst->ino = st->st_ino;
	myst->mode = from_platform_mode(st->st_mode);
	myst->nlink = st->st_nlink;
	myst->uid = st->st_uid;
	myst->gid = st->st_gid;
	myst->size = st->st_size;
	myst->blksize = st->st_blksize;
	myst->atime = st->st_atime;
	myst->mtime = st->st_mtime;
	myst->ctime = st->st_ctime;
	myst->crtime = st->st_ctime;
}


void
to_platform_stat(const struct my_stat *myst, struct stat *st)
{
	st->st_dev = myst->dev;
	st->st_ino = myst->ino;
	st->st_mode = to_platform_mode(myst->mode);
	st->st_nlink = myst->nlink;
	st->st_uid = myst->uid;
	st->st_gid = myst->gid;
	st->st_size = myst->size;
	st->st_blksize = myst->blksize;
	st->st_atime = myst->atime;
	st->st_mtime = myst->mtime;
	st->st_ctime = myst->ctime;
//	st->st_crtime = myst->crtime;
}


int
to_platform_open_mode(int myMode)
{
	#define SET_OPEN_MODE_FLAG(flag, myFlag)	\
		if (myMode & myFlag)			\
			mode |= flag;

	int mode = 0;

	// the r/w mode
	switch (myMode & MY_O_RWMASK) {
		case MY_O_RDONLY:
			mode |= O_RDONLY;
			break;
		case MY_O_WRONLY:
			mode |= O_WRONLY;
			break;
		case MY_O_RDWR:
			mode |= O_RDWR;
			break;
	}

	// the flags
	//SET_OPEN_MODE_FLAG(O_CLOEXEC, MY_O_CLOEXEC)
	SET_OPEN_MODE_FLAG(O_NONBLOCK, MY_O_NONBLOCK)
	SET_OPEN_MODE_FLAG(O_EXCL, MY_O_EXCL)
	SET_OPEN_MODE_FLAG(O_CREAT, MY_O_CREAT)
	SET_OPEN_MODE_FLAG(O_TRUNC, MY_O_TRUNC)
	SET_OPEN_MODE_FLAG(O_APPEND, MY_O_APPEND)
	SET_OPEN_MODE_FLAG(O_NOCTTY, MY_O_NOCTTY)
	SET_OPEN_MODE_FLAG(O_NOTRAVERSE, MY_O_NOTRAVERSE)
	//SET_OPEN_MODE_FLAG(O_TEXT, MY_O_TEXT)
	//SET_OPEN_MODE_FLAG(O_BINARY, MY_O_BINARY)

	#undef SET_OPEN_MODE_FLAG

	return mode;
}

#endif // !__BEOS__
