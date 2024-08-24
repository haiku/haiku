/**
 * Copyright (c) 2005-2007 Yura Pakhuchiy
 * Copyright (c) 2005 Yuval Fledel
 * Copyright (c) 2006-2009 Szabolcs Szakacsits
 * Copyright (c) 2007-2021 Jean-Pierre Andre
 * Copyright (c) 2009 Erik Larsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the NTFS-3G
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "lowntfs.h"

#include <stdlib.h>
#include <errno.h>

#include "libntfs/dir.h"
#include "libntfs/reparse.h"
#include "libntfs/misc.h"


#define DISABLE_PLUGINS
#define KERNELPERMS 1
#define INODE(ino) ino
#define ntfs_allowed_dir_access(...) (1) /* permissions checks done elsewhere */

#define set_archive(ni) (ni)->flags |= FILE_ATTR_ARCHIVE


static const char ntfs_bad_reparse[] = "unsupported reparse tag 0x%08lx";
	 /* exact length of target text, without the terminator */
#define ntfs_bad_reparse_lth (sizeof(ntfs_bad_reparse) + 2)

static const char ghostformat[] = ".ghost-ntfs-3g-%020llu";
#define GHOSTLTH 40 /* max length of a ghost file name - see ghostformat */

static u32 ntfs_sequence = 0;


static void
set_fuse_error(int *err)
{
	if (!*err)
		*err = errno;
}

static void
ntfs_fuse_update_times(ntfs_inode *ni, ntfs_time_update_flags mask)
{
#ifdef __HAIKU__
	mask &= ~NTFS_UPDATE_ATIME;
#else
	if (ctx->atime == ATIME_DISABLED)
		mask &= ~NTFS_UPDATE_ATIME;
	else if (ctx->atime == ATIME_RELATIVE && mask == NTFS_UPDATE_ATIME &&
			(sle64_to_cpu(ni->last_access_time)
				>= sle64_to_cpu(ni->last_data_change_time)) &&
			(sle64_to_cpu(ni->last_access_time)
				>= sle64_to_cpu(ni->last_mft_change_time)))
		return;
#endif
	ntfs_inode_update_times(ni, mask);
}


static BOOL ntfs_fuse_fill_security_context(struct lowntfs_context *ctx,
			struct SECURITY_CONTEXT *scx)
{
	const struct fuse_ctx *fusecontext;

	scx->vol = ctx->vol;
	scx->mapping[MAPUSERS] = NULL;
	scx->mapping[MAPGROUPS] = NULL;
	scx->pseccache = NULL;
	scx->uid = 0;
	scx->gid = 0;
	scx->tid = 0;
	scx->umask = 0;
	return (scx->mapping[MAPUSERS] != (struct MAPPING*)NULL);
}


u64
ntfs_fuse_inode_lookup(struct lowntfs_context *ctx, u64 parent, const char *name)
{
	u64 ino = (u64)-1;
	u64 inum;
	ntfs_inode *dir_ni;

	/* Open target directory. */
	dir_ni = ntfs_inode_open(ctx->vol, INODE(parent));
	if (dir_ni) {
		/* Lookup file */
		inum = ntfs_inode_lookup_by_mbsname(dir_ni, name);
			/* never return inodes 0 and 1 */
		if (MREF(inum) <= 1) {
			inum = (u64)-1;
			errno = ENOENT;
		}
		if (ntfs_inode_close(dir_ni)
			|| (inum == (u64)-1))
			ino = (u64)-1;
		else
			ino = MREF(inum);
	}
	return (ino);
}


int
ntfs_fuse_getstat(struct lowntfs_context *ctx, struct SECURITY_CONTEXT *scx,
	ntfs_inode *ni, struct stat *stbuf)
{
	int res = 0;
	ntfs_attr *na;
	BOOL withusermapping;

	memset(stbuf, 0, sizeof(struct stat));
	withusermapping = scx && (scx->mapping[MAPUSERS] != (struct MAPPING*)NULL);
	stbuf->st_nlink = le16_to_cpu(ni->mrec->link_count);
	if (ctx->posix_nlink
		&& !(ni->flags & FILE_ATTR_REPARSE_POINT))
		stbuf->st_nlink = ntfs_dir_link_cnt(ni);
	if ((ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)
		|| (ni->flags & FILE_ATTR_REPARSE_POINT)) {
		if (ni->flags & FILE_ATTR_REPARSE_POINT) {
#ifndef DISABLE_PLUGINS
			const plugin_operations_t *ops;
			REPARSE_POINT *reparse;

			res = CALL_REPARSE_PLUGIN(ni, getattr, stbuf);
			if (!res) {
				apply_umask(stbuf);
			} else {
				stbuf->st_size = ntfs_bad_reparse_lth;
				stbuf->st_blocks =
					(ni->allocated_size + 511) >> 9;
				stbuf->st_mode = S_IFLNK;
				res = 0;
			}
			goto ok;
#else /* DISABLE_PLUGINS */
			char *target;

			errno = 0;
			target = ntfs_make_symlink(ni, ctx->abs_mnt_point);
				/*
				 * If the reparse point is not a valid
				 * directory junction, and there is no error
				 * we still display as a symlink
				 */
			if (target || (errno == EOPNOTSUPP)) {
				if (target)
					stbuf->st_size = strlen(target);
				else
					stbuf->st_size = ntfs_bad_reparse_lth;
				stbuf->st_blocks =
					(ni->allocated_size + 511) >> 9;
				stbuf->st_nlink =
					le16_to_cpu(ni->mrec->link_count);
				stbuf->st_mode = S_IFLNK;
				free(target);
			} else {
				res = -errno;
				goto exit;
			}
#endif /* DISABLE_PLUGINS */
		} else {
			/* Directory. */
			stbuf->st_mode = S_IFDIR | (0777 & ~ctx->dmask);
			/* get index size, if not known */
			if (!test_nino_flag(ni, KnownSize)) {
				na = ntfs_attr_open(ni, AT_INDEX_ALLOCATION,
						NTFS_INDEX_I30, 4);
				if (na) {
					ni->data_size = na->data_size;
					ni->allocated_size = na->allocated_size;
					set_nino_flag(ni, KnownSize);
					ntfs_attr_close(na);
				}
			}
			stbuf->st_size = ni->data_size;
			stbuf->st_blocks = ni->allocated_size >> 9;
			if (!ctx->posix_nlink)
				stbuf->st_nlink = 1;	/* Make find(1) work */
		}
	} else {
		/* Regular or Interix (INTX) file. */
		stbuf->st_mode = S_IFREG;
		stbuf->st_size = ni->data_size;
#ifdef HAVE_SETXATTR	/* extended attributes interface required */
		/*
		 * return data size rounded to next 512 byte boundary for
		 * encrypted files to include padding required for decryption
		 * also include 2 bytes for padding info
		*/
		if (ctx->efs_raw
			&& (ni->flags & FILE_ATTR_ENCRYPTED)
			&& ni->data_size)
			stbuf->st_size = ((ni->data_size + 511) & ~511) + 2;
#endif /* HAVE_SETXATTR */
		/*
		 * Temporary fix to make ActiveSync work via Samba 3.0.
		 * See more on the ntfs-3g-devel list.
		 */
		stbuf->st_blocks = (ni->allocated_size + 511) >> 9;
		if (ni->flags & FILE_ATTR_SYSTEM) {
			na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
			if (!na) {
				stbuf->st_ino = ni->mft_no;
				goto nodata;
			}
			/* Check whether it's Interix FIFO or socket. */
			if (!(ni->flags & FILE_ATTR_HIDDEN)) {
				/* FIFO. */
				if (na->data_size == 0)
					stbuf->st_mode = S_IFIFO;
				/* Socket link. */
				if (na->data_size == 1)
					stbuf->st_mode = S_IFSOCK;
			}
			/*
			 * Check whether it's Interix symbolic link, block or
			 * character device.
			 */
			if ((u64)na->data_size <= sizeof(INTX_FILE_TYPES)
					+ sizeof(ntfschar) * PATH_MAX
				&& (u64)na->data_size >
					sizeof(INTX_FILE_TYPES)) {
				INTX_FILE *intx_file;

				intx_file =
					(INTX_FILE*)ntfs_malloc(na->data_size);
				if (!intx_file) {
					res = -errno;
					ntfs_attr_close(na);
					goto exit;
				}
				if (ntfs_attr_pread(na, 0, na->data_size,
						intx_file) != na->data_size) {
					res = -errno;
					free(intx_file);
					ntfs_attr_close(na);
					goto exit;
				}
				if (intx_file->magic == INTX_BLOCK_DEVICE &&
						na->data_size == (s64)offsetof(
						INTX_FILE, device_end)) {
					stbuf->st_mode = S_IFBLK;
					stbuf->st_rdev = makedev(le64_to_cpu(
							intx_file->major),
							le64_to_cpu(
							intx_file->minor));
				}
				if (intx_file->magic == INTX_CHARACTER_DEVICE &&
						na->data_size == (s64)offsetof(
						INTX_FILE, device_end)) {
					stbuf->st_mode = S_IFCHR;
					stbuf->st_rdev = makedev(le64_to_cpu(
							intx_file->major),
							le64_to_cpu(
							intx_file->minor));
				}
				if (intx_file->magic == INTX_SYMBOLIC_LINK) {
					char *target = NULL;
					int len;

					/* st_size should be set to length of
					 * symlink target as multibyte string */
					len = ntfs_ucstombs(
							intx_file->target,
							(na->data_size -
								offsetof(INTX_FILE,
									 target)) /
								   sizeof(ntfschar),
								 &target, 0);
					if (len < 0) {
						res = -errno;
						free(intx_file);
						ntfs_attr_close(na);
						goto exit;
					}
					free(target);
					stbuf->st_mode = S_IFLNK;
					stbuf->st_size = len;
				}
				free(intx_file);
			}
			ntfs_attr_close(na);
		}
		stbuf->st_mode |= (0777 & ~ctx->fmask);
	}
#ifndef DISABLE_PLUGINS
ok:
#endif /* DISABLE_PLUGINS */
	if (withusermapping) {
		ntfs_get_owner_mode(scx,ni,stbuf);
	} else {
#ifndef __HAIKU__
		stbuf->st_uid = ctx->uid;
		stbuf->st_gid = ctx->gid;
#endif
	}
	if (S_ISLNK(stbuf->st_mode))
		stbuf->st_mode |= 0777;
nodata :
	stbuf->st_ino = ni->mft_no;
#ifdef HAVE_STRUCT_STAT_ST_ATIMESPEC
	stbuf->st_atimespec = ntfs2timespec(ni->last_access_time);
	stbuf->st_ctimespec = ntfs2timespec(ni->last_mft_change_time);
	stbuf->st_mtimespec = ntfs2timespec(ni->last_data_change_time);
#elif defined(HAVE_STRUCT_STAT_ST_ATIM)
	stbuf->st_atim = ntfs2timespec(ni->last_access_time);
	stbuf->st_ctim = ntfs2timespec(ni->last_mft_change_time);
	stbuf->st_mtim = ntfs2timespec(ni->last_data_change_time);
#elif defined(HAVE_STRUCT_STAT_ST_ATIMENSEC)
	{
	struct timespec ts;

	ts = ntfs2timespec(ni->last_access_time);
	stbuf->st_atime = ts.tv_sec;
	stbuf->st_atimensec = ts.tv_nsec;
	ts = ntfs2timespec(ni->last_mft_change_time);
	stbuf->st_ctime = ts.tv_sec;
	stbuf->st_ctimensec = ts.tv_nsec;
	ts = ntfs2timespec(ni->last_data_change_time);
	stbuf->st_mtime = ts.tv_sec;
	stbuf->st_mtimensec = ts.tv_nsec;
	}
#else
#warning "No known way to set nanoseconds in struct stat !"
	{
	struct timespec ts;

	ts = ntfs2timespec(ni->last_access_time);
	stbuf->st_atime = ts.tv_sec;
	ts = ntfs2timespec(ni->last_mft_change_time);
	stbuf->st_ctime = ts.tv_sec;
	ts = ntfs2timespec(ni->last_data_change_time);
	stbuf->st_mtime = ts.tv_sec;
	}
#endif
#ifdef __HAIKU__
	stbuf->st_crtim = ntfs2timespec(ni->creation_time);
#endif
exit:
	return (res);
}


int
ntfs_fuse_readlink(struct lowntfs_context* ctx, u64 ino, void* buffer, size_t* bufferSize)
{
	ntfs_inode *ni = NULL;
	ntfs_attr *na = NULL;
	INTX_FILE *intx_file = NULL;
	char *buf = (char*)NULL;
	int res = 0;

	/* Get inode. */
	ni = ntfs_inode_open(ctx->vol, INODE(ino));
	if (!ni) {
		res = -errno;
		goto exit;
	}
		/*
		 * Reparse point : analyze as a junction point
		 */
	if (ni->flags & FILE_ATTR_REPARSE_POINT) {
		REPARSE_POINT *reparse;
		le32 tag;
		int lth;
#ifndef DISABLE_PLUGINS
		const plugin_operations_t *ops;

		res = CALL_REPARSE_PLUGIN(ni, readlink, &buf);
			/* plugin missing or reparse tag failing the check */
		if (res && ((errno == ELIBACC) || (errno == EINVAL)))
			errno = EOPNOTSUPP;
#else /* DISABLE_PLUGINS */
		errno = 0;
		res = 0;
		buf = ntfs_make_symlink(ni, ctx->abs_mnt_point);
#endif /* DISABLE_PLUGINS */
		if (!buf && (errno == EOPNOTSUPP)) {
			buf = (char*)malloc(ntfs_bad_reparse_lth + 1);
			if (buf) {
				reparse = ntfs_get_reparse_point(ni);
				if (reparse) {
					tag = reparse->reparse_tag;
					free(reparse);
				} else
					tag = const_cpu_to_le32(0);
				lth = snprintf(buf, ntfs_bad_reparse_lth + 1,
						ntfs_bad_reparse,
						(long)le32_to_cpu(tag));
				res = 0;
				if (lth != ntfs_bad_reparse_lth) {
					free(buf);
					buf = (char*)NULL;
				}
			}
		}
		if (!buf)
			res = -errno;
		goto exit;
	}
	/* Sanity checks. */
	if (!(ni->flags & FILE_ATTR_SYSTEM)) {
		res = -EINVAL;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
	if (!na) {
		res = -errno;
		goto exit;
	}
	if ((size_t)na->data_size <= sizeof(INTX_FILE_TYPES)) {
		res = -EINVAL;
		goto exit;
	}
	if ((size_t)na->data_size > sizeof(INTX_FILE_TYPES) +
			sizeof(ntfschar) * PATH_MAX) {
		res = -ENAMETOOLONG;
		goto exit;
	}
	/* Receive file content. */
	intx_file = (INTX_FILE*)ntfs_malloc(na->data_size);
	if (!intx_file) {
		res = -errno;
		goto exit;
	}
	if (ntfs_attr_pread(na, 0, na->data_size, intx_file) != na->data_size) {
		res = -errno;
		goto exit;
	}
	/* Sanity check. */
	if (intx_file->magic != INTX_SYMBOLIC_LINK) {
		res = -EINVAL;
		goto exit;
	}
	/* Convert link from unicode to local encoding. */
	if (ntfs_ucstombs(intx_file->target, (na->data_size -
			offsetof(INTX_FILE, target)) / sizeof(ntfschar),
			&buf, 0) < 0) {
		res = -errno;
		goto exit;
	}
exit:
	if (intx_file)
		free(intx_file);
	if (na)
		ntfs_attr_close(na);
	ntfs_inode_close(ni);

#ifdef __HAIKU__
	if (buf && !res) {
		strlcpy(buffer, buf, *bufferSize);
		*bufferSize = strlen(buf);
			// Indicate the actual length of the link.
	}
#endif

	free(buf);
	return res;
}


int
ntfs_fuse_read(ntfs_inode* ni, off_t offset, char* buf, size_t size)
{
	ntfs_attr *na = NULL;
	int res;
	s64 total = 0;
	s64 max_read;

	if (!size) {
		res = 0;
		goto exit;
	}

	if (ni->flags & FILE_ATTR_REPARSE_POINT) {
#ifndef DISABLE_PLUGINS
		const plugin_operations_t *ops;
		REPARSE_POINT *reparse;
		struct open_file *of;

		of = (struct open_file*)(long)fi->fh;
		res = CALL_REPARSE_PLUGIN(ni, read, buf, size, offset, &of->fi);
		if (res >= 0) {
			goto stamps;
		}
#else /* DISABLE_PLUGINS */
		errno = EOPNOTSUPP;
		res = errno;
#endif /* DISABLE_PLUGINS */
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
	if (!na) {
		res = errno;
		goto exit;
	}
	max_read = na->data_size;
#ifdef HAVE_SETXATTR	/* extended attributes interface required */
	/* limit reads at next 512 byte boundary for encrypted attributes */
	if (ctx->efs_raw
		&& max_read
		&& (na->data_flags & ATTR_IS_ENCRYPTED)
		&& NAttrNonResident(na)) {
		max_read = ((na->data_size+511) & ~511) + 2;
	}
#endif /* HAVE_SETXATTR */
	if (offset + (off_t)size > max_read) {
		if (max_read < offset)
			goto ok;
		size = max_read - offset;
	}
	while (size > 0) {
		s64 ret = ntfs_attr_pread(na, offset, size, buf + total);
		if (ret != (s64)size)
			ntfs_log_perror("ntfs_attr_pread error reading inode %lld at "
				"offset %lld: %lld <> %lld", (long long)ni->mft_no,
				(long long)offset, (long long)size, (long long)ret);
		if (ret <= 0 || ret > (s64)size) {
			res = (ret < 0) ? errno : EIO;
			goto exit;
		}
		size -= ret;
		offset += ret;
		total += ret;
	}
ok:
	res = total;
#ifndef DISABLE_PLUGINS
stamps :
#endif /* DISABLE_PLUGINS */
	ntfs_fuse_update_times(ni, NTFS_UPDATE_ATIME);
exit:
	if (na)
		ntfs_attr_close(na);
	return res;
}

int
ntfs_fuse_write(struct lowntfs_context* ctx, ntfs_inode* ni, const char *buf,
	size_t size, off_t offset)
{
	ntfs_attr *na = NULL;
	int res, total = 0;

	if (ni->flags & FILE_ATTR_REPARSE_POINT) {
#ifndef DISABLE_PLUGINS
		const plugin_operations_t *ops;
		REPARSE_POINT *reparse;
		struct open_file *of;

		of = (struct open_file*)(long)fi->fh;
		res = CALL_REPARSE_PLUGIN(ni, write, buf, size, offset,
								&of->fi);
		if (res >= 0) {
			goto stamps;
		}
#else /* DISABLE_PLUGINS */
		res = EOPNOTSUPP;
		errno = EOPNOTSUPP;
#endif /* DISABLE_PLUGINS */
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
	if (!na) {
		res = errno;
		goto exit;
	}
	while (size) {
		s64 ret = ntfs_attr_pwrite(na, offset, size, buf + total);
		if (ret <= 0) {
			res = errno;
			goto exit;
		}
		size   -= ret;
		offset += ret;
		total  += ret;
	}
	res = total;
#ifndef DISABLE_PLUGINS
stamps :
#endif /* DISABLE_PLUGINS */
	if ((res > 0)
		&& (!ctx->dmtime
		|| (sle64_to_cpu(ntfs_current_time())
			 - sle64_to_cpu(ni->last_data_change_time)) > ctx->dmtime))
		ntfs_fuse_update_times(ni, NTFS_UPDATE_MCTIME);
exit:
	if (na)
		ntfs_attr_close(na);
	if (res > 0)
		set_archive(ni);
	return res;
}

int
ntfs_fuse_create(struct lowntfs_context *ctx, ino_t parent, const char *name,
	mode_t typemode, dev_t dev, const char *target, ino_t* ino)
{
	ntfschar *uname = NULL, *utarget = NULL;
	ntfs_inode *dir_ni = NULL, *ni;
	struct open_file *of;
	int state = 0;
	le32 securid;
	gid_t gid;
	mode_t dsetgid;
	mode_t type = typemode & ~07777;
	mode_t perm;
	struct SECURITY_CONTEXT security;
	int res = 0, uname_len, utarget_len;
	const int fi = 1;

	/* Generate unicode filename. */
	uname_len = ntfs_mbstoucs(name, &uname);
	if ((uname_len < 0)
		|| (ctx->windows_names
		&& ntfs_forbidden_names(ctx->vol,uname,uname_len,TRUE))) {
		res = -errno;
		goto exit;
	}
	/* Deny creating into $Extend */
	if (parent == FILE_Extend) {
		errno = EPERM;
		res = -errno;
		goto exit;
	}
	/* Open parent directory. */
	dir_ni = ntfs_inode_open(ctx->vol, INODE(parent));
	if (!dir_ni) {
		res = -errno;
		goto exit;
	}
#if !KERNELPERMS | (POSIXACLS & !KERNELACLS)
		/* make sure parent directory is writeable and executable */
	if (!ntfs_fuse_fill_security_context(ctx, &security)
		   || ntfs_allowed_create(&security,
				dir_ni, &gid, &dsetgid)) {
#else
		ntfs_fuse_fill_security_context(ctx, &security);
		ntfs_allowed_create(&security, dir_ni, &gid, &dsetgid);
#endif
		if (S_ISDIR(type))
			perm = (typemode & ~ctx->dmask & 0777)
				| (dsetgid & S_ISGID);
		else
			if ((ctx->special_files == NTFS_FILES_WSL)
				&& S_ISLNK(type))
				perm = typemode | 0777;
			else
				perm = typemode & ~ctx->fmask & 0777;
			/*
			 * Try to get a security id available for
			 * file creation (from inheritance or argument).
			 * This is not possible for NTFS 1.x, and we will
			 * have to build a security attribute later.
			 */
		if (!security.mapping[MAPUSERS])
			securid = const_cpu_to_le32(0);
		else
			if (ctx->inherit)
				securid = ntfs_inherited_id(&security,
					dir_ni, S_ISDIR(type));
			else
#if POSIXACLS
				securid = ntfs_alloc_securid(&security,
					security.uid, gid,
					dir_ni, perm, S_ISDIR(type));
#else
				securid = ntfs_alloc_securid(&security,
					security.uid, gid,
					perm & ~security.umask, S_ISDIR(type));
#endif
		/* Create object specified in @type. */
		if (dir_ni->flags & FILE_ATTR_REPARSE_POINT) {
#ifndef DISABLE_PLUGINS
			const plugin_operations_t *ops;
			REPARSE_POINT *reparse;

			reparse = (REPARSE_POINT*)NULL;
			ops = select_reparse_plugin(ctx, dir_ni, &reparse);
			if (ops && ops->create) {
				ni = (*ops->create)(dir_ni, reparse,
					securid, uname, uname_len, type);
			} else {
				ni = (ntfs_inode*)NULL;
				errno = EOPNOTSUPP;
			}
			free(reparse);
#else /* DISABLE_PLUGINS */
			ni = (ntfs_inode*)NULL;
			errno = EOPNOTSUPP;
			res = -errno;
#endif /* DISABLE_PLUGINS */
		} else {
			switch (type) {
				case S_IFCHR:
				case S_IFBLK:
					ni = ntfs_create_device(dir_ni, securid,
							uname, uname_len,
							type, dev);
					break;
				case S_IFLNK:
					utarget_len = ntfs_mbstoucs(target,
							&utarget);
					if (utarget_len < 0) {
						res = -errno;
						goto exit;
					}
					ni = ntfs_create_symlink(dir_ni,
							securid,
							uname, uname_len,
							utarget, utarget_len);
					break;
				default:
					ni = ntfs_create(dir_ni, securid, uname,
							uname_len, type);
					break;
			}
		}
		if (ni) {
				/*
				 * set the security attribute if a security id
				 * could not be allocated (eg NTFS 1.x)
				 */
			if (security.mapping[MAPUSERS]) {
#if POSIXACLS
				if (!securid
				   && ntfs_set_inherited_posix(&security, ni,
					security.uid, gid,
					dir_ni, perm) < 0)
					set_fuse_error(&res);
#else
				if (!securid
				   && ntfs_set_owner_mode(&security, ni,
					security.uid, gid,
					perm & ~security.umask) < 0)
					set_fuse_error(&res);
#endif
			}
			set_archive(ni);
			/* mark a need to compress the end of file */
			if (fi && (ni->flags & FILE_ATTR_COMPRESSED)) {
				state |= CLOSE_COMPRESSED;
			}
#ifdef HAVE_SETXATTR	/* extended attributes interface required */
			/* mark a future need to fixup encrypted inode */
			if (fi
				&& ctx->efs_raw
				&& (ni->flags & FILE_ATTR_ENCRYPTED))
				state |= CLOSE_ENCRYPTED;
#endif /* HAVE_SETXATTR */
			if (fi && ctx->dmtime)
				state |= CLOSE_DMTIME;
			ntfs_inode_update_mbsname(dir_ni, name, ni->mft_no);
			NInoSetDirty(ni);
			*ino = ni->mft_no;
#ifndef __HAIKU__
			e->ino = ni->mft_no;
			e->generation = 1;
			e->attr_timeout = ATTR_TIMEOUT;
			e->entry_timeout = ENTRY_TIMEOUT;
			res = ntfs_fuse_getstat(&security, ni, &e->attr);
#endif
			/*
			 * closing ni requires access to dir_ni to
			 * synchronize the index, avoid double opening.
			 */
			if (ntfs_inode_close_in_dir(ni, dir_ni))
				set_fuse_error(&res);
			ntfs_fuse_update_times(dir_ni, NTFS_UPDATE_MCTIME);
		} else
			res = -errno;
#if !KERNELPERMS | (POSIXACLS & !KERNELACLS)
	} else
		res = -errno;
#endif

exit:
	free(uname);
	if (ntfs_inode_close(dir_ni))
		set_fuse_error(&res);
	if (utarget)
		free(utarget);
#ifndef __HAIKU__
	if ((res >= 0) && fi) {
		of = (struct open_file*)malloc(sizeof(struct open_file));
		if (of) {
			of->parent = 0;
			of->ino = e->ino;
			of->state = state;
			of->next = ctx->open_files;
			of->previous = (struct open_file*)NULL;
			if (ctx->open_files)
				ctx->open_files->previous = of;
			ctx->open_files = of;
			fi->fh = (long)of;
		}
	}
#else
	// FIXME: store "state" somewhere
#endif
	return res;
}

static int ntfs_fuse_newlink(struct lowntfs_context *ctx,
		ino_t ino, ino_t newparent,
		const char *newname, struct fuse_entry_param *e)
{
	ntfschar *uname = NULL;
	ntfs_inode *dir_ni = NULL, *ni;
	int res = 0, uname_len;
	struct SECURITY_CONTEXT security;

	/* Open file for which create hard link. */
	ni = ntfs_inode_open(ctx->vol, INODE(ino));
	if (!ni) {
		res = -errno;
		goto exit;
	}

	/* Do not accept linking to a directory (except for renaming) */
	if (e && (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)) {
		errno = EPERM;
		res = -errno;
		goto exit;
	}
	/* Generate unicode filename. */
	uname_len = ntfs_mbstoucs(newname, &uname);
	if ((uname_len < 0)
			|| (ctx->windows_names
				&& ntfs_forbidden_names(ctx->vol,uname,uname_len,TRUE))) {
		res = -errno;
		goto exit;
	}
	/* Open parent directory. */
	dir_ni = ntfs_inode_open(ctx->vol, INODE(newparent));
	if (!dir_ni) {
		res = -errno;
		goto exit;
	}

#if !KERNELPERMS | (POSIXACLS & !KERNELACLS)
		/* make sure the target parent directory is writeable */
	if (ntfs_fuse_fill_security_context(ctx, &security)
		&& !ntfs_allowed_access(&security,dir_ni,S_IWRITE + S_IEXEC))
		res = -EACCES;
	else
#else
	ntfs_fuse_fill_security_context(ctx, &security);
#endif
		{
		if (dir_ni->flags & FILE_ATTR_REPARSE_POINT) {
#ifndef DISABLE_PLUGINS
			const plugin_operations_t *ops;
			REPARSE_POINT *reparse;

			res = CALL_REPARSE_PLUGIN(dir_ni, link,
					ni, uname, uname_len);
			if (res < 0)
				goto exit;
#else /* DISABLE_PLUGINS */
			res = -EOPNOTSUPP;
			goto exit;
#endif /* DISABLE_PLUGINS */
		} else {
			if (ntfs_link(ni, dir_ni, uname, uname_len)) {
				res = -errno;
				goto exit;
			}
		}
		ntfs_inode_update_mbsname(dir_ni, newname, ni->mft_no);
#ifndef __HAIKU__
		if (e) {
			e->ino = ni->mft_no;
			e->generation = 1;
			e->attr_timeout = ATTR_TIMEOUT;
			e->entry_timeout = ENTRY_TIMEOUT;
			res = ntfs_fuse_getstat(&security, ni, &e->attr);
		}
#endif
		set_archive(ni);
		ntfs_fuse_update_times(ni, NTFS_UPDATE_CTIME);
		ntfs_fuse_update_times(dir_ni, NTFS_UPDATE_MCTIME);
	}
exit:
	/*
	 * Must close dir_ni first otherwise ntfs_inode_sync_file_name(ni)
	 * may fail because ni may not be in parent's index on the disk yet.
	 */
	if (ntfs_inode_close(dir_ni))
		set_fuse_error(&res);
	if (ntfs_inode_close(ni))
		set_fuse_error(&res);
	free(uname);
	return (res);
}


int
ntfs_fuse_rm(struct lowntfs_context *ctx, ino_t parent, const char *name,
	enum RM_TYPES rm_type)
{
	ntfschar *uname = NULL;
	ntfschar *ugname;
	ntfs_inode *dir_ni = NULL, *ni = NULL;
	int res = 0, uname_len;
	int ugname_len;
	u64 iref;
	ino_t ino;
	char ghostname[GHOSTLTH];
#if !KERNELPERMS | (POSIXACLS & !KERNELACLS)
	struct SECURITY_CONTEXT security;
#endif
#if defined(__sun) && defined (__SVR4)
	int isdir;
#endif /* defined(__sun) && defined (__SVR4) */
	int no_check_open = (rm_type & RM_NO_CHECK_OPEN) != 0;
	rm_type &= ~RM_NO_CHECK_OPEN;

	/* Deny removing from $Extend */
	if (parent == FILE_Extend) {
		res = -EPERM;
		goto exit;
	}
	/* Open parent directory. */
	dir_ni = ntfs_inode_open(ctx->vol, INODE(parent));
	if (!dir_ni) {
		res = -errno;
		goto exit;
	}
	/* Generate unicode filename. */
	uname_len = ntfs_mbstoucs(name, &uname);
	if (uname_len < 0) {
		res = -errno;
		goto exit;
	}
	/* Open object for delete. */
	iref = ntfs_inode_lookup_by_mbsname(dir_ni, name);
	if (iref == (u64)-1) {
		res = -errno;
		goto exit;
	}
	ino = (ino_t)MREF(iref);
	/* deny unlinking metadata files */
	if (ino < FILE_first_user) {
		errno = EPERM;
		res = -errno;
		goto exit;
	}

	ni = ntfs_inode_open(ctx->vol, ino);
	if (!ni) {
		res = -errno;
		goto exit;
	}

#if defined(__sun) && defined (__SVR4)
	/* on Solaris : deny unlinking directories */
	isdir = ni->mrec->flags & MFT_RECORD_IS_DIRECTORY;
#ifndef DISABLE_PLUGINS
		/* get emulated type from plugin if available */
	if (ni->flags & FILE_ATTR_REPARSE_POINT) {
		struct stat st;
		const plugin_operations_t *ops;
		REPARSE_POINT *reparse;

			/* Avoid double opening of parent directory */
		res = ntfs_inode_close(dir_ni);
		if (res)
			goto exit;
		dir_ni = (ntfs_inode*)NULL;
		res = CALL_REPARSE_PLUGIN(ni, getattr, &st);
		if (res)
			goto exit;
		isdir = S_ISDIR(st.st_mode);
		dir_ni = ntfs_inode_open(ctx->vol, INODE(parent));
		if (!dir_ni) {
			res = -errno;
			goto exit;
		}
	}
#endif /* DISABLE_PLUGINS */
	if (rm_type == (isdir ? RM_LINK : RM_DIR)) {
		errno = (isdir ? EISDIR : ENOTDIR);
		res = -errno;
		goto exit;
	}
#endif /* defined(__sun) && defined (__SVR4) */

#if !KERNELPERMS | (POSIXACLS & !KERNELACLS)
	/* JPA deny unlinking if directory is not writable and executable */
	if (ntfs_fuse_fill_security_context(ctx, &security)
		&& !ntfs_allowed_dir_access(&security, dir_ni, ino, ni,
				   S_IEXEC + S_IWRITE + S_ISVTX)) {
		errno = EACCES;
		res = -errno;
		goto exit;
	}
#endif
#ifndef __HAIKU__
		/*
		 * We keep one open_file record per opening, to avoid
		 * having to check the list of open files when opening
		 * and closing (which are more frequent than unlinking).
		 * As a consequence, we may have to create several
		 * ghosts names for the same file.
		 * The file may have been opened with a different name
		 * in a different parent directory. The ghost is
		 * nevertheless created in the parent directory of the
		 * name being unlinked, and permissions to do so are the
		 * same as required for unlinking.
		 */
	for (of=ctx->open_files; of; of = of->next) {
		if ((of->ino == ino) && !(of->state & CLOSE_GHOST)) {
#else
	int* close_state = NULL;
	if (!no_check_open) {
		close_state = ntfs_haiku_get_close_state(ctx, ino);
		if (close_state && (*close_state & CLOSE_GHOST) == 0) {
#endif
			/* file was open, create a ghost in unlink parent */
			ntfs_inode *gni;
			u64 gref;

			/* ni has to be closed for linking ghost */
			if (ni) {
				if (ntfs_inode_close(ni)) {
					res = -errno;
					goto exit;
				}
				ni = (ntfs_inode*)NULL;
			}
			*close_state |= CLOSE_GHOST;
			u64 ghost = ++ctx->latest_ghost;

			sprintf(ghostname,ghostformat,ghost);
				/* Generate unicode filename. */
			ugname = (ntfschar*)NULL;
			ugname_len = ntfs_mbstoucs(ghostname, &ugname);
			if (ugname_len < 0) {
				res = -errno;
				goto exit;
			}
			/* sweep existing ghost if any, ignoring errors */
			gref = ntfs_inode_lookup_by_mbsname(dir_ni, ghostname);
			if (gref != (u64)-1) {
				gni = ntfs_inode_open(ctx->vol, MREF(gref));
				ntfs_delete(ctx->vol, (char*)NULL, gni, dir_ni,
					 ugname, ugname_len);
				/* ntfs_delete() always closes gni and dir_ni */
				dir_ni = (ntfs_inode*)NULL;
			} else {
				if (ntfs_inode_close(dir_ni)) {
					res = -errno;
					goto out;
				}
				dir_ni = (ntfs_inode*)NULL;
			}
			free(ugname);
			res = ntfs_fuse_newlink(ctx, ino, parent, ghostname,
					(struct fuse_entry_param*)NULL);
			if (res)
				goto out;

#ifdef __HAIKU__
			// We have to do this before the parent directory is reopened.
			ntfs_haiku_put_close_state(ctx, ino, ghost);
			close_state = NULL;
#endif

				/* now reopen then parent directory */
			dir_ni = ntfs_inode_open(ctx->vol, INODE(parent));
			if (!dir_ni) {
				res = -errno;
				goto exit;
			}
		}
	}
	if (!ni) {
		ni = ntfs_inode_open(ctx->vol, ino);
		if (!ni) {
			res = -errno;
			goto exit;
		}
	}
	if (dir_ni->flags & FILE_ATTR_REPARSE_POINT) {
#ifndef DISABLE_PLUGINS
		const plugin_operations_t *ops;
		REPARSE_POINT *reparse;

		res = CALL_REPARSE_PLUGIN(dir_ni, unlink, (char*)NULL,
				ni, uname, uname_len);
#else /* DISABLE_PLUGINS */
		errno = EOPNOTSUPP;
		res = -errno;
#endif /* DISABLE_PLUGINS */
	} else {
		if (ntfs_delete(ctx->vol, (char*)NULL, ni, dir_ni,
					 uname, uname_len))
			res = -errno;
		/* ntfs_delete() always closes ni and dir_ni */
	}
	ni = dir_ni = NULL;
exit:
	if (ntfs_inode_close(ni) && !res)
		res = -errno;
	if (ntfs_inode_close(dir_ni) && !res)
		res = -errno;
out :
#ifdef __HAIKU__
	if (close_state)
		ntfs_haiku_put_close_state(ctx, ino, -1);
#endif
	free(uname);
	return res;
}

int ntfs_fuse_release(struct lowntfs_context *ctx, ino_t parent, ino_t ino, int state, u64 ghost)
{
	ntfs_inode *ni = NULL;
	ntfs_attr *na = NULL;
	char ghostname[GHOSTLTH];
	int res;

	/* Only for marked descriptors there is something to do */
	if (!(state & (CLOSE_COMPRESSED | CLOSE_ENCRYPTED
				| CLOSE_DMTIME | CLOSE_REPARSE))) {
		res = 0;
		goto out;
	}
	ni = ntfs_inode_open(ctx->vol, INODE(ino));
	if (!ni) {
		res = -errno;
		goto exit;
	}
	if (ni->flags & FILE_ATTR_REPARSE_POINT) {
#ifndef DISABLE_PLUGINS
		const plugin_operations_t *ops;
		REPARSE_POINT *reparse;

		res = CALL_REPARSE_PLUGIN(ni, release, &of->fi);
		if (!res) {
			goto stamps;
		}
#else /* DISABLE_PLUGINS */
			/* Assume release() was not needed */
		res = 0;
#endif /* DISABLE_PLUGINS */
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
	if (!na) {
		res = -errno;
		goto exit;
	}
	res = 0;
	if (state & CLOSE_COMPRESSED)
		res = ntfs_attr_pclose(na);
#ifdef HAVE_SETXATTR	/* extended attributes interface required */
	if (of->state & CLOSE_ENCRYPTED)
		res = ntfs_efs_fixup_attribute(NULL, na);
#endif /* HAVE_SETXATTR */
#ifndef DISABLE_PLUGINS
stamps :
#endif /* DISABLE_PLUGINS */
	if (state & CLOSE_DMTIME)
		ntfs_inode_update_times(ni,NTFS_UPDATE_MCTIME);
exit:
	if (na)
		ntfs_attr_close(na);
	if (ntfs_inode_close(ni))
		set_fuse_error(&res);
out:
		/* remove the associate ghost file (even if release failed) */
	if (1) {
		if (state & CLOSE_GHOST) {
			sprintf(ghostname,ghostformat,ghost);
			ntfs_fuse_rm(ctx, parent, ghostname, RM_ANY | RM_NO_CHECK_OPEN);
		}
#ifndef __HAIKU__
			/* remove from open files list */
		if (of->next)
			of->next->previous = of->previous;
		if (of->previous)
			of->previous->next = of->next;
		else
			ctx->open_files = of->next;
		free(of);
#endif
	}
#ifndef __HAIKU__
	if (res)
		fuse_reply_err(req, -res);
	else
		fuse_reply_err(req, 0);
#endif
	return res;
}

static int ntfs_fuse_safe_rename(struct lowntfs_context *ctx, ino_t ino,
			ino_t parent, const char *name, ino_t xino,
			ino_t newparent, const char *newname,
			const char *tmp)
{
	int ret;

	ntfs_log_trace("Entering\n");

	ret = ntfs_fuse_newlink(ctx, xino, newparent, tmp,
				(struct fuse_entry_param*)NULL);
	if (ret)
		return ret;

	ret = ntfs_fuse_rm(ctx, newparent, newname, RM_ANY);
	if (!ret) {

		ret = ntfs_fuse_newlink(ctx, ino, newparent, newname,
					(struct fuse_entry_param*)NULL);
		if (ret)
			goto restore;

		ret = ntfs_fuse_rm(ctx, parent, name, RM_ANY | RM_NO_CHECK_OPEN);
		if (ret) {
			if (ntfs_fuse_rm(ctx, newparent, newname, RM_ANY))
				goto err;
			goto restore;
		}
	}

	goto cleanup;
restore:
	if (ntfs_fuse_newlink(ctx, xino, newparent, newname,
				(struct fuse_entry_param*)NULL)) {
err:
		ntfs_log_perror("Rename failed. Existing file '%s' was renamed "
				"to '%s'", newname, tmp);
	} else {
cleanup:
		/*
		 * Condition for this unlink has already been checked in
		 * "ntfs_fuse_rename_existing_dest()", so it should never
		 * fail (unless concurrent access to directories when fuse
		 * is multithreaded)
		 */
		if (ntfs_fuse_rm(ctx, newparent, tmp, RM_ANY) < 0)
			ntfs_log_perror("Rename failed. Existing file '%s' still present "
				"as '%s'", newname, tmp);
	}
	return	ret;
}

static int ntfs_fuse_rename_existing_dest(struct lowntfs_context *ctx, ino_t ino,
			ino_t parent, const char *name,
			ino_t xino, ino_t newparent,
			const char *newname)
{
	int ret, len;
	char *tmp;
	const char *ext = ".ntfs-3g-";
#if !KERNELPERMS | (POSIXACLS & !KERNELACLS)
	ntfs_inode *newdir_ni;
	struct SECURITY_CONTEXT security;
#endif

	ntfs_log_trace("Entering\n");

	len = strlen(newname) + strlen(ext) + 10 + 1; /* wc(str(2^32)) + \0 */
	tmp = (char*)ntfs_malloc(len);
	if (!tmp)
		return -errno;

	ret = snprintf(tmp, len, "%s%s%010d", newname, ext, ++ntfs_sequence);
	if (ret != len - 1) {
		ntfs_log_error("snprintf failed: %d != %d\n", ret, len - 1);
		ret = -EOVERFLOW;
	} else {
#if !KERNELPERMS | (POSIXACLS & !KERNELACLS)
			/*
			 * Make sure existing dest can be removed.
			 * This is only needed if parent directory is
			 * sticky, because in this situation condition
			 * for unlinking is different from condition for
			 * linking
			 */
		newdir_ni = ntfs_inode_open(ctx->vol, INODE(newparent));
		if (newdir_ni) {
			if (!ntfs_fuse_fill_security_context(ctx,&security)
				|| ntfs_allowed_dir_access(&security, newdir_ni,
					xino, (ntfs_inode*)NULL,
					S_IEXEC + S_IWRITE + S_ISVTX)) {
				if (ntfs_inode_close(newdir_ni))
					ret = -errno;
				else
					ret = ntfs_fuse_safe_rename(ctx, ino,
							parent, name, xino,
							newparent, newname,
							tmp);
			} else {
				ntfs_inode_close(newdir_ni);
				ret = -EACCES;
			}
		} else
			ret = -errno;
#else
		ret = ntfs_fuse_safe_rename(ctx, ino, parent, name,
					xino, newparent, newname, tmp);
#endif
	}
	free(tmp);
	return	ret;
}


int ntfs_fuse_rename(struct lowntfs_context *ctx, ino_t parent,
			const char *name, ino_t newparent,
			const char *newname)
{
	int ret;
	ino_t ino;
	ino_t xino;
	ntfs_inode *ni;

	ntfs_log_debug("rename: old: '%s'  new: '%s'\n", name, newname);

	/*
	 *  FIXME: Rename should be atomic.
	 */

	ino = ntfs_fuse_inode_lookup(ctx, parent, name);
	if (ino == (ino_t)-1) {
		ret = -errno;
		goto out;
	}
	/* Check whether target is present */
	xino = ntfs_fuse_inode_lookup(ctx, newparent, newname);
	if (xino != (ino_t)-1) {
			/*
			 * Target exists : no need to check whether it
			 * designates the same inode, this has already
			 * been checked (by fuse ?)
			 */
		ni = ntfs_inode_open(ctx->vol, INODE(xino));
		if (!ni)
			ret = -errno;
		else {
			ret = ntfs_check_empty_dir(ni);
			if (ret < 0) {
				ret = -errno;
				ntfs_inode_close(ni);
				goto out;
			}

			if (ntfs_inode_close(ni)) {
				set_fuse_error(&ret);
				goto out;
			}
			ret = ntfs_fuse_rename_existing_dest(ctx, ino, parent,
						name, xino, newparent, newname);
		}
	} else {
			/* target does not exist */
		ret = ntfs_fuse_newlink(ctx, ino, newparent, newname,
					(struct fuse_entry_param*)NULL);
		if (ret)
			goto out;

		ret = ntfs_fuse_rm(ctx, parent, name, RM_ANY | RM_NO_CHECK_OPEN);
		if (ret)
			ntfs_fuse_rm(ctx, newparent, newname, RM_ANY);
	}
out:
#ifndef __HAIKU__
	if (ret)
		fuse_reply_err(req, -ret);
	else
		fuse_reply_err(req, 0);
#endif
	return ret;
}
