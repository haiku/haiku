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

#include <errno.h>

#include "libntfs/dir.h"


#define DISABLE_PLUGINS
#define INODE(ino) ino

static const char ntfs_bad_reparse[] = "unsupported reparse tag 0x%08lx";
	 /* exact length of target text, without the terminator */
#define ntfs_bad_reparse_lth (sizeof(ntfs_bad_reparse) + 2)


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
