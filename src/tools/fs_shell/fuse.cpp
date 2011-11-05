/*
 * Copyright 2009, Raghuram Nagireddy <raghuram87@gmail.com>.
 * Distributed under the terms of the MIT License.
 */

#define FUSE_USE_VERSION 27

#include <fuse/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

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

#define PRINTD(x) if (gIsDebug) fprintf(stderr, x)

bool gIsDebug = false;

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

#define _ERR(x) (-1 * fssh_to_host_error(x))


// pragma mark - FUSE functions


int
fuse_getattr(const char* path, struct stat* stbuf)
{
	PRINTD("##getattr\n");
	struct fssh_stat f_stbuf;
	fssh_status_t status = _kern_read_stat(-1, path, false, &f_stbuf,
			sizeof(f_stbuf));
	fromFsshStatToStat(&f_stbuf, stbuf);
	if (gIsDebug)
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
	fssh_size_t n_size = size - 1;
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


static fssh_dev_t
get_volume_id()
{
	struct fssh_stat st;
	fssh_status_t error = _kern_read_stat(-1, kMountPoint, false, &st,
		sizeof(st));
	if (error != FSSH_B_OK)
		return error;
	return st.fssh_st_dev;
}


static int
fuse_statfs(const char *path __attribute__((unused)),
                            struct statvfs *sfs)
{
	PRINTD("##statfs\n");

	fssh_dev_t volumeID = get_volume_id();
	if (volumeID < 0)
		return _ERR(volumeID);

	fssh_fs_info info;
	fssh_status_t status = _kern_read_fs_info(volumeID, &info);
	if (status != FSSH_B_OK)
		return _ERR(status);

	sfs->f_bsize = sfs->f_frsize = info.block_size;
	sfs->f_blocks = info.total_blocks;
	sfs->f_bavail = sfs->f_bfree = info.free_blocks;

	return 0;
}


struct fuse_operations gFUSEOperations;


static void
initialiseFuseOps(struct fuse_operations* fuseOps)
{
	fuseOps->getattr	= fuse_getattr;
	fuseOps->access		= fuse_access;
	fuseOps->readlink	= fuse_readlink;
	fuseOps->readdir	= fuse_readdir;
	fuseOps->mknod		= fuse_mknod;
	fuseOps->mkdir		= fuse_mkdir;
	fuseOps->symlink	= fuse_symlink;
	fuseOps->unlink		= fuse_unlink;
	fuseOps->rmdir		= fuse_rmdir;
	fuseOps->rename		= fuse_rename;
	fuseOps->link		= fuse_link;
	fuseOps->chmod		= fuse_chmod;
	fuseOps->chown		= fuse_chown;
	fuseOps->truncate	= NULL;
	fuseOps->utimens	= NULL;
	fuseOps->open		= fuse_open;
	fuseOps->read		= fuse_read;
	fuseOps->write		= fuse_write;
	fuseOps->statfs		= fuse_statfs;
	fuseOps->release	= NULL;
	fuseOps->fsync		= NULL;
	fuseOps->destroy	= fuse_destroy;
}


static int
mount_volume(const char* device, const char* mntPoint, const char* fsName)
{
	// Mount the volume in the root FS.
	fssh_dev_t fsDev = _kern_mount(kMountPoint, device, fsName, 0, NULL, 0);
	if (fsDev < 0) {
		fprintf(stderr, "Error: Mounting FS failed: %s\n",
			fssh_strerror(fsDev));
		return 1;
	}

	if (!gIsDebug) {
		bool isErr = false;
		fssh_dev_t volumeID = get_volume_id();
		if (volumeID < 0)
			isErr = true;
		fssh_fs_info info;
		if (!isErr) {
			fssh_status_t status = _kern_read_fs_info(volumeID, &info);
			if (status != FSSH_B_OK)
				isErr = true;
		}
		syslog(LOG_INFO, "Mounted %s (%s) to %s",
			device,
			isErr ? "unknown" : info.volume_name,
			mntPoint);
	}
	
	return 0;
}


static int
unmount_volume(const char* device, const char* mntPoint)
{
	// Unmount the volume again.
	// Avoid a "busy" vnode.
	_kern_setcwd(-1, "/");
	fssh_status_t error = _kern_unmount(kMountPoint, 0);
	if (error != FSSH_B_OK) {
		if (gIsDebug)
			fprintf(stderr, "Error: Unmounting FS failed: %s\n",
				fssh_strerror(error));
		else
			syslog(LOG_INFO, "Error: Unmounting FS failed: %s",
				fssh_strerror(error));
		return 1;
	}

	if (!gIsDebug)
		syslog(LOG_INFO, "UnMounted %s from %s", device, mntPoint);
	
	return 0;
}


static int
fssh_fuse_session(const char* device, const char* mntPoint, const char* fsName,
	struct fuse_args& fuseArgs)
{
	int ret;
	
	ret = mount_volume(device, mntPoint, fsName);
	if (ret != 0)
		return ret;
	
	char* fuseOptions = NULL;

	// default FUSE options
	char* fsNameOption = NULL;
	if (fuse_opt_add_opt(&fuseOptions, "allow_other") < 0
		|| asprintf(&fsNameOption, "fsname=%s", device) < 0
		|| fuse_opt_add_opt(&fuseOptions, fsNameOption) < 0) {
		unmount_volume(device, mntPoint);
		return 1;
	}

	struct stat sbuf;
	if ((stat(device, &sbuf) == 0) && S_ISBLK(sbuf.st_mode)) {
		int blkSize = 512;
		fssh_dev_t volumeID = get_volume_id();
		if (volumeID >= 0) {
			fssh_fs_info info;
			if (_kern_read_fs_info(volumeID, &info) == FSSH_B_OK)
				blkSize = info.block_size;
		}

		char* blkSizeOption = NULL;
		if (fuse_opt_add_opt(&fuseOptions, "blkdev") < 0
			|| asprintf(&blkSizeOption, "blksize=%i", blkSize) < 0
			|| fuse_opt_add_opt(&fuseOptions, blkSizeOption) < 0) {
			unmount_volume(device, mntPoint);
			return 1;
		}
	}

 	// Run the fuse_main() loop.
	if (fuse_opt_add_arg(&fuseArgs, "-s") < 0
		|| fuse_opt_add_arg(&fuseArgs, "-o") < 0
		|| fuse_opt_add_arg(&fuseArgs, fuseOptions) < 0) {
		unmount_volume(device, mntPoint);
		return 1;
	}

	initialiseFuseOps(&gFUSEOperations);

	int res = fuse_main(fuseArgs.argc, fuseArgs.argv, &gFUSEOperations, NULL);

	ret = unmount_volume(device, mntPoint);
	if (ret != 0)
		return ret;

	return res;
}


}	// namespace FSShell


using namespace FSShell;


static void
print_usage_and_exit(const char* binName)
{
	fprintf(stderr,"Usage: %s [-d] <device> <mount point>\n", binName);
	exit(1);
}


struct FsConfig {
	const char* device;
	const char* mntPoint;
};


enum {
	KEY_DEBUG,
	KEY_HELP
};


static int
process_options(void* data, const char* arg, int key, struct fuse_args* outArgs)
{
	struct FsConfig* config = (FsConfig*) data;

	switch (key) {
		case FUSE_OPT_KEY_NONOPT:
			if (!config->device) {
				config->device = arg;
				return 0;
					// don't pass the device path to fuse_main()
			} else if (!config->mntPoint)
				config->mntPoint = arg;
			else
				print_usage_and_exit(outArgs->argv[0]);
			break;
		case KEY_DEBUG:
			gIsDebug = true;
			break;
		case KEY_HELP:
			print_usage_and_exit(outArgs->argv[0]);
	}

	return 1;
}


int
main(int argc, char* argv[])
{
	struct fuse_args fuseArgs = FUSE_ARGS_INIT(argc, argv);
	struct FsConfig config;
	memset(&config, 0, sizeof(config));
	const struct fuse_opt fsOptions[] = {
		FUSE_OPT_KEY("uhelper=",	FUSE_OPT_KEY_DISCARD),
			// fuse_main() throws an error about this unknown option
			// TODO: do not use fuse_main to mount filesystem, instead use
			// fuse_mount, fuse_new, fuse_set_signal_handlers and fuse_loop
		FUSE_OPT_KEY("-d",			KEY_DEBUG),
		FUSE_OPT_KEY("-h",			KEY_HELP),
		FUSE_OPT_KEY("--help",		KEY_HELP),
		FUSE_OPT_END
	};

	if (fuse_opt_parse(&fuseArgs, &config, fsOptions, process_options) < 0)
		return 1;

	if (!config.mntPoint)
		print_usage_and_exit(fuseArgs.argv[0]);

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
	return fssh_fuse_session(config.device, config.mntPoint, fsName, fuseArgs);
}

