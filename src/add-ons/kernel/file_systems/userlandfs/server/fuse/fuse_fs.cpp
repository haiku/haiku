/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "fuse_fs.h"

#include <string.h>

#include <new>

#include "Debug.h"


int
fuse_fs_getattr(struct fuse_fs* fs, const char* path, struct stat* buf)
{
	if (fs->ops.getattr == NULL)
		return ENOSYS;
	return fs->ops.getattr(path, buf);
}


int
fuse_fs_fgetattr(struct fuse_fs* fs, const char* path, struct stat* buf,
	struct fuse_file_info* fi)
{
	if (fs->ops.fgetattr == NULL)
		return ENOSYS;
	return fs->ops.fgetattr(path, buf, fi);
}


int
fuse_fs_rename(struct fuse_fs* fs, const char* oldpath, const char* newpath)
{
	if (fs->ops.rename == NULL)
		return ENOSYS;
	return fs->ops.rename(oldpath, newpath);
}


int
fuse_fs_unlink(struct fuse_fs* fs, const char* path)
{
	if (fs->ops.unlink == NULL)
		return ENOSYS;
	return fs->ops.unlink(path);
}


int
fuse_fs_rmdir(struct fuse_fs* fs, const char* path)
{
	if (fs->ops.rmdir == NULL)
		return ENOSYS;
	return fs->ops.rmdir(path);
}


int
fuse_fs_symlink(struct fuse_fs* fs, const char* linkname, const char* path)
{
	if (fs->ops.symlink == NULL)
		return ENOSYS;
	return fs->ops.symlink(linkname, path);
}


int
fuse_fs_link(struct fuse_fs* fs, const char* oldpath, const char* newpath)
{
	if (fs->ops.link == NULL)
		return ENOSYS;
	return fs->ops.link(oldpath, newpath);
}


int
fuse_fs_release(struct fuse_fs* fs,	 const char* path,
	struct fuse_file_info* fi)
{
	if (fs->ops.release == NULL)
		return 0;
	return fs->ops.release(path, fi);
}


int
fuse_fs_open(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi)
{
	if (fs->ops.open == NULL)
		return 0;
	return fs->ops.open(path, fi);
}


int
fuse_fs_read(struct fuse_fs* fs, const char* path, char *buf, size_t size,
	off_t off, struct fuse_file_info* fi)
{
	if (fs->ops.read == NULL)
		return ENOSYS;
	return fs->ops.read(path, buf, size, off, fi);
}


int
fuse_fs_write(struct fuse_fs* fs, const char* path, const char* buf,
	size_t size, off_t off, struct fuse_file_info* fi)
{
	if (fs->ops.write == NULL)
		return ENOSYS;
	return fs->ops.write(path, buf, size, off, fi);
}


int
fuse_fs_fsync(struct fuse_fs* fs, const char* path, int datasync,
	struct fuse_file_info* fi)
{
	if (fs->ops.fsync == NULL)
		return ENOSYS;
	return fs->ops.fsync(path, datasync, fi);
}


int
fuse_fs_flush(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi)
{
	if (fs->ops.flush == NULL)
		return ENOSYS;
	return fs->ops.flush(path, fi);
}


int
fuse_fs_statfs(struct fuse_fs* fs, const char* path, struct statvfs* buf)
{
	if (fs->ops.statfs == NULL)
		return 0;
	return fs->ops.statfs(path, buf);
}


int
fuse_fs_opendir(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi)
{
	if (fs->ops.opendir == NULL)
		return 0;
	return fs->ops.opendir(path, fi);
}


int
fuse_fs_readdir(struct fuse_fs* fs, const char* path, void* buf,
	fuse_fill_dir_t filler, off_t off, struct fuse_file_info* fi)
{
	if (fs->ops.readdir == NULL)
		return ENOSYS;
	return fs->ops.readdir(path, buf, filler, off, fi);
}


int
fuse_fs_fsyncdir(struct fuse_fs* fs, const char* path, int datasync,
	struct fuse_file_info* fi)
{
	if (fs->ops.fsyncdir == NULL)
		return ENOSYS;
	return fs->ops.fsyncdir(path, datasync, fi);
}


int
fuse_fs_releasedir(struct fuse_fs* fs, const char* path,
	struct fuse_file_info* fi)
{
	if (fs->ops.releasedir == NULL)
		return 0;
	return fs->ops.releasedir(path, fi);
}


int
fuse_fs_create(struct fuse_fs* fs, const char* path, mode_t mode,
	struct fuse_file_info* fi)
{
	if (fs->ops.create == NULL)
		return ENOSYS;
	return fs->ops.create(path, mode, fi);
}


int
fuse_fs_lock(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi,
	int cmd, struct flock* lock)
{
	if (fs->ops.lock == NULL)
		return ENOSYS;
	return fs->ops.lock(path, fi, cmd, lock);
}


int
fuse_fs_chmod(struct fuse_fs* fs, const char* path, mode_t mode)
{
	if (fs->ops.chmod == NULL)
		return ENOSYS;
	return fs->ops.chmod(path, mode);
}


int
fuse_fs_chown(struct fuse_fs* fs, const char* path, uid_t uid, gid_t gid)
{
	if (fs->ops.chown == NULL)
		return ENOSYS;
	return fs->ops.chown(path, uid, gid);
}


int
fuse_fs_truncate(struct fuse_fs* fs, const char* path, off_t size)
{
	if (fs->ops.truncate == NULL)
		return ENOSYS;
	return fs->ops.truncate(path, size);
}


int
fuse_fs_ftruncate(struct fuse_fs* fs, const char* path, off_t size,
	struct fuse_file_info* fi)
{
	if (fs->ops.ftruncate == NULL)
		return ENOSYS;
	return fs->ops.ftruncate(path, size, fi);
}


int
fuse_fs_utimens(struct fuse_fs* fs, const char* path,
	const struct timespec tv[2])
{
	if (fs->ops.utimens != NULL)
		return fs->ops.utimens(path, tv);

	if (fs->ops.utime != NULL) {
		utimbuf timeBuffer = {
			tv[0].tv_sec,	// access time
			tv[1].tv_sec	// modification time
		};
		return fs->ops.utime(path, &timeBuffer);
	}

	return ENOSYS;
}


int
fuse_fs_access(struct fuse_fs* fs, const char* path, int mask)
{
	if (fs->ops.access == NULL)
		return ENOSYS;
	return fs->ops.access(path, mask);
}


int
fuse_fs_readlink(struct fuse_fs* fs, const char* path, char* buf, size_t len)
{
	if (fs->ops.readlink == NULL)
		return ENOSYS;
	return fs->ops.readlink(path, buf, len);
}


int
fuse_fs_mknod(struct fuse_fs* fs, const char* path, mode_t mode, dev_t rdev)
{
	if (fs->ops.mknod == NULL)
		return ENOSYS;
	return fs->ops.mknod(path, mode, rdev);
}


int
fuse_fs_mkdir(struct fuse_fs* fs, const char* path, mode_t mode)
{
	if (fs->ops.mkdir == NULL)
		return ENOSYS;
	return fs->ops.mkdir(path, mode);
}


int
fuse_fs_setxattr(struct fuse_fs* fs, const char* path, const char* name,
	const char* value, size_t size, int flags)
{
	if (fs->ops.setxattr == NULL)
		return ENOSYS;
	return fs->ops.setxattr(path, name, value, size, flags);
}


int
fuse_fs_getxattr(struct fuse_fs* fs, const char* path, const char* name,
	char* value, size_t size)
{
	if (fs->ops.getxattr == NULL)
		return ENOSYS;
	return fs->ops.getxattr(path, name, value, size);
}


int
fuse_fs_listxattr(struct fuse_fs* fs, const char* path, char* list, size_t size)
{
	if (fs->ops.listxattr == NULL)
		return ENOSYS;
	return fs->ops.listxattr(path, list, size);
}


int
fuse_fs_removexattr(struct fuse_fs* fs, const char* path, const char* name)
{
	if (fs->ops.removexattr == NULL)
		return ENOSYS;
	return fs->ops.removexattr(path, name);
}


int
fuse_fs_bmap(struct fuse_fs* fs, const char* path, size_t blocksize,
	uint64_t* idx)
{
	if (fs->ops.bmap == NULL)
		return ENOSYS;
	return fs->ops.bmap(path, blocksize, idx);
}


void
fuse_fs_init(struct fuse_fs* fs, struct fuse_conn_info* conn)
{
	if (fs->ops.init == NULL)
		return;
	fs->ops.init(conn);
}


void
fuse_fs_destroy(struct fuse_fs* fs)
{
	if (fs->ops.destroy != NULL)
		fs->ops.destroy(fs->userData);

	delete fs;
}


struct fuse_fs*
fuse_fs_new(const struct fuse_operations* ops, size_t opSize, void* userData)
{
	if (sizeof(fuse_operations) < opSize) {
		ERROR(("fuse_fs_new(): Client FS built with newer library version!\n"));
		return NULL;
	}

	fuse_fs* fs = new(std::nothrow) fuse_fs;
	if (fs == NULL)
		return NULL;

	memset(&fs->ops, 0, sizeof(fuse_operations));
	memcpy(&fs->ops, ops, opSize);

	fs->userData = userData;

	return fs;
}


int
fuse_fs_get_fs_info(struct fuse_fs* fs, struct fs_info* info)
{
	if (fs->ops.get_fs_info == NULL)
		return ENOSYS;
	return fs->ops.get_fs_info(info);
}
