/*
 * Copyright 2009, Raghuram Nagireddy <raghuram87@gmail.com>.
 * Distributed under the terms of the MIT License.
 */

#define FUSE_USE_VERSION 27

#include <fuse/fuse.h>
#include <stdio.h>
#include <stdlib.h>

#include "fssh.h"

#include "driver_settings.h"
#include "external_commands.h"
#include "fd.h"
#include "fssh_dirent.h"
#include "fssh_errno.h"
#include "fssh_errors.h"
#include "fssh_fcntl.h"
#include "fssh_fs_info.h"
#include "fssh_module.h"
#include "fssh_node_monitor.h"
#include "fssh_stat.h"
#include "fssh_string.h"
#include "fssh_type_constants.h"
#include "module.h"
#include "syscalls.h"
#include "vfs.h"


extern fssh_module_info *modules[];

extern fssh_file_system_module_info gRootFileSystem;

namespace FSShell {

const char* kMountPoint = "/myfs";

static mode_t sUmask = 0022;

#define PRINTD(x) fprintf(stderr, x)


static fssh_status_t
init_kernel()
{
	fssh_status_t error;

	// init module subsystem
	error = module_init(NULL);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "module_init() failed: %s\n", fssh_strerror(error));
		return error;
	}

	// init driver settings
	error = driver_settings_init();
	if (error != FSSH_B_OK) {
		fprintf(stderr, "initializing driver settings failed: %s\n",
			fssh_strerror(error));
		return error;
	}

	// register built-in modules, i.e. the rootfs and the client FS
	register_builtin_module(&gRootFileSystem.info);
	for (int i = 0; modules[i]; i++)
		register_builtin_module(modules[i]);

	// init VFS
	error = vfs_init(NULL);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "initializing VFS failed: %s\n", fssh_strerror(error));
		return error;
	}

	// init kernel IO context
	gKernelIOContext = (io_context*)vfs_new_io_context(NULL);
	if (!gKernelIOContext) {
		fprintf(stderr, "creating IO context failed!\n");
		return FSSH_B_NO_MEMORY;
	}

	// mount root FS
	fssh_dev_t rootDev = _kern_mount("/", NULL, "rootfs", 0, NULL, 0);
	if (rootDev < 0) {
		fprintf(stderr, "mounting rootfs failed: %s\n", fssh_strerror(rootDev));
		return rootDev;
	}

	// set cwd to "/"
	error = _kern_setcwd(-1, "/");
	if (error != FSSH_B_OK) {
		fprintf(stderr, "setting cwd failed: %s\n", fssh_strerror(error));
		return error;
	}

	// create mount point for the client FS
	error = _kern_create_dir(-1, kMountPoint, 0775);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "creating mount point failed: %s\n",
			fssh_strerror(error));
		return error;
	}

	return FSSH_B_OK;
}


static void
fromFsshStatToStat(struct fssh_stat* f_stbuf, struct stat* stbuf)
{
	stbuf->st_dev = f_stbuf->fssh_st_dev;
	stbuf->st_ino = f_stbuf->fssh_st_ino;
	stbuf->st_mode = f_stbuf->fssh_st_mode;
	stbuf->st_nlink = f_stbuf->fssh_st_nlink;
	stbuf->st_uid = f_stbuf->fssh_st_uid;
	stbuf->st_gid = f_stbuf->fssh_st_gid;
	stbuf->st_rdev = f_stbuf->fssh_st_rdev;
	stbuf->st_size = f_stbuf->fssh_st_size;
	stbuf->st_blksize = f_stbuf->fssh_st_blksize;
	stbuf->st_blocks = f_stbuf->fssh_st_blocks;
	stbuf->st_atime = f_stbuf->fssh_st_atime;
	stbuf->st_mtime = f_stbuf->fssh_st_mtime;
	stbuf->st_ctime = f_stbuf->fssh_st_ctime;
}

#define _ERR(x) (-1*fssh_to_host_error(x))


// pragma mark - FUSE functions


//extern "C" {


int
fuse_getattr(const char* path, struct stat* stbuf)
{
	PRINTD("##getattr\n");
	struct fssh_stat f_stbuf;
	fssh_status_t status = _kern_read_stat(-1, path, false, &f_stbuf,
			sizeof(f_stbuf));
	fromFsshStatToStat(&f_stbuf, stbuf);
	printf("GETATTR returned: %d\n", status);
	return _ERR(status);
}


static int
fuse_access(const char* path, int mask)
{
	PRINTD("##access\n");
	return _ERR(_kern_access(path, mask));
}


static int
fuse_readlink(const char* path, char* buffer, size_t size)
{
	PRINTD("##readlink\n");
	size_t n_size = size - 1;
	fssh_status_t st = _kern_read_link(-1, path, buffer, &n_size);
	if (st >= FSSH_B_OK)
		buffer[n_size] = '\0';
	return _ERR(st);
}


static int
fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info* fi)
{
	PRINTD("##readdir\n");
	int dfp = _kern_open_dir(-1, path);
	if (dfp < FSSH_B_OK)
		return _ERR(dfp);

	fssh_ssize_t entriesRead = 0;
	struct fssh_stat f_st;
	struct stat st;
	char buffer[sizeof(fssh_dirent) + FSSH_B_FILE_NAME_LENGTH];
	fssh_dirent* dirEntry = (fssh_dirent*)buffer;
	while ((entriesRead = _kern_read_dir(dfp, dirEntry,
			sizeof(buffer), 1)) == 1) {
		fssh_memset(&st, 0, sizeof(st));
		fssh_memset(&f_st, 0, sizeof(f_st));
		fssh_status_t status = _kern_read_stat(dfp, dirEntry->d_name,
			false, &f_st, sizeof(f_st));
		if (status >= FSSH_B_OK) {
			fromFsshStatToStat(&f_st, &st);
			if (filler(buf, dirEntry->d_name, &st, 0))
				break;
		}
	}
	_kern_close(dfp);
	//TODO: check _kern_close
	return 0;
}


static int
fuse_mknod(const char* path, mode_t mode, dev_t rdev)
{
	PRINTD("##mknod\n");
	if (S_ISREG(mode)) {
		int fd = _kern_open(-1, path,
			FSSH_O_CREAT | FSSH_O_EXCL | FSSH_O_WRONLY, mode);
		if (fd >= FSSH_B_OK)
			return _ERR(_kern_close(fd));
		return _ERR(fd);
	} else if (S_ISFIFO(mode))
		return _ERR(FSSH_EINVAL);
	else
		return _ERR(FSSH_EINVAL);
}


static int
fuse_mkdir(const char* path, mode_t mode)
{
	PRINTD("##mkdir\n");
	return _ERR(_kern_create_dir(-1, path, mode));
}


static int
fuse_symlink(const char* from, const char* to)
{
	PRINTD("##symlink\n");
	return _ERR(_kern_create_symlink(-1, to, from,
		FSSH_S_IRWXU | FSSH_S_IRWXG | FSSH_S_IRWXO));
}


static int
fuse_unlink(const char* path)
{
	PRINTD("##unlink\n");
	return _ERR(_kern_unlink(-1, path));
}


static int
fuse_rmdir(const char* path)
{
	PRINTD("##rmdir\n");
	return _ERR(_kern_remove_dir(-1, path));
}


static int
fuse_rename(const char* from, const char* to)
{
	PRINTD("##rename\n");
	return _ERR(_kern_rename(-1, from, -1, to));
}


static int
fuse_link(const char* from, const char* to)
{
	PRINTD("##link\n");
	return _ERR(_kern_create_link(to, from));
}


static int
fuse_chmod(const char* path, mode_t mode)
{
	PRINTD("##chmod\n");
	fssh_struct_stat st;
	st.fssh_st_mode = mode;
	return _ERR(_kern_write_stat(-1, path, false, &st, sizeof(st),
			FSSH_B_STAT_MODE));
}


static int
fuse_chown(const char* path, uid_t uid, gid_t gid)
{
	PRINTD("##chown\n");
	fssh_struct_stat st;
	st.fssh_st_uid = uid;
	st.fssh_st_gid = gid;
	return _ERR(_kern_write_stat(-1, path, false, &st, sizeof(st),
			FSSH_B_STAT_UID|FSSH_B_STAT_GID));
}


static int
fuse_open(const char* path, struct fuse_file_info* fi)
{
	PRINTD("##open\n");
	// TODO: Do we have a syscall similar to the open syscall in linux which
	// takes only two args: path and flags with no mask/perms?
	int fd = _kern_open(-1, path, fi->flags,
		(FSSH_S_IRWXU | FSSH_S_IRWXG | FSSH_S_IRWXO) & ~sUmask);
	_kern_close(fd);
	if (fd < FSSH_B_OK)
		return _ERR(fd);
	else
		return 0;
}


static int
fuse_read(const char* path, char* buf, size_t size, off_t offset,
	struct fuse_file_info* fi)
{
	PRINTD("##read\n");
	int fd = _kern_open(-1, path, FSSH_O_RDONLY,
		(FSSH_S_IRWXU | FSSH_S_IRWXG | FSSH_S_IRWXO) & ~sUmask);
	if (fd < FSSH_B_OK)
		return _ERR(fd);

	int res = _kern_read(fd, offset, buf, size);
	_kern_close(fd);
	if (res < FSSH_B_OK)
		res = _ERR(res);
	return res;
}


static int
fuse_write(const char* path, const char* buf, size_t size, off_t offset,
	struct fuse_file_info* fi)
{
	PRINTD("##write\n");
	int fd = _kern_open(-1, path, FSSH_O_WRONLY,
		(FSSH_S_IRWXU | FSSH_S_IRWXG | FSSH_S_IRWXO) & ~sUmask);
	if (fd < FSSH_B_OK)
		return _ERR(fd);

	int res = _kern_write(fd, offset, buf, size);
	_kern_close(fd);
	if (res < FSSH_B_OK)
		res = _ERR(res);
	return res;
}


static void
fuse_destroy(void* priv_data)
{
	_kern_sync();
}


//} // extern "C" {


struct fuse_operations fuse_cmds;


static void
initialiseFuseOps(struct fuse_operations* fuse_cmds)
{
	fuse_cmds->getattr	= fuse_getattr;
	fuse_cmds->access	= fuse_access;
	fuse_cmds->readlink	= fuse_readlink;
	fuse_cmds->readdir	= fuse_readdir;
	fuse_cmds->mknod	= fuse_mknod;
	fuse_cmds->mkdir	= fuse_mkdir;
	fuse_cmds->symlink	= fuse_symlink;
	fuse_cmds->unlink	= fuse_unlink;
	fuse_cmds->rmdir	= fuse_rmdir;
	fuse_cmds->rename	= fuse_rename;
	fuse_cmds->link		= fuse_link;
	fuse_cmds->chmod	= fuse_chmod;
	fuse_cmds->chown	= fuse_chown;
	fuse_cmds->truncate	= NULL;
	fuse_cmds->utimens	= NULL;
	fuse_cmds->open		= fuse_open;
	fuse_cmds->read		= fuse_read;
	fuse_cmds->write	= fuse_write;
	fuse_cmds->statfs	= NULL;
	fuse_cmds->release	= NULL;
	fuse_cmds->fsync	= NULL;
	fuse_cmds->destroy	= fuse_destroy;
}


static int
fssh_fuse_session(const char* device, const char* mntPoint, const char* fsName)
{
	fssh_dev_t fsDev = _kern_mount(kMountPoint, device, fsName, 0, NULL, 0);
	if (fsDev < 0) {
		fprintf(stderr, "Error: Mounting FS failed: %s\n",
			fssh_strerror(fsDev));
		return 1;
	}
	const char* argv[5];
	argv[0] = (const char*)"bfs_shell";
	argv[1] = mntPoint;
	argv[2] = (const char*)"-d";
	argv[3] = (const char*)"-s";
	initialiseFuseOps(&fuse_cmds);
	return fuse_main(4, (char**)argv, &fuse_cmds, NULL);
}


}	// namespace FSShell


using namespace FSShell;


static void
print_usage_and_exit(const char* binName)
{
	fprintf(stderr,"Usage: %s <device> <mount point>\n", binName);
	exit(1);
}


int
main(int argc, const char* const* argv)
{
	if (argc < 2)
		print_usage_and_exit(argv[0]);

	const char* device = argv[1];
	const char* mntPoint = argv[2];

	if (!modules[0]) {
		fprintf(stderr, "Error: Couldn't find FS module!\n");
		return 1;
	}

	fssh_status_t error = init_kernel();
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Initializing kernel failed: %s\n",
			fssh_strerror(error));
		return error;
	}
	const char* fsName = modules[0]->name;
	return fssh_fuse_session(device, mntPoint, fsName);
}

