/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/statvfs.h>
#include <sys/stat.h>

#include <fs_info.h>


static int
fill_statvfs(dev_t device, struct statvfs *statvfs)
{
	fs_info info;
	if (fs_stat_dev(device, &info) < 0)
		return -1;

	statvfs->f_frsize = statvfs->f_bsize = info.block_size;
	statvfs->f_blocks = info.total_blocks;
	statvfs->f_bavail = statvfs->f_bfree = info.free_blocks;
	statvfs->f_files = info.total_nodes;
	statvfs->f_favail = statvfs->f_ffree = info.free_nodes;
	statvfs->f_fsid = device;
	statvfs->f_flag = info.flags & B_FS_IS_READONLY ? ST_RDONLY : 0;
	statvfs->f_namemax = B_FILE_NAME_LENGTH;

	return 0;
}


//	#pragma mark -


int
statvfs(const char *path, struct statvfs *statvfs)
{
	dev_t device = dev_for_path(path);
	if (device < 0)
		return -1;

	return fill_statvfs(device, statvfs);
}


int
fstatvfs(int fd, struct statvfs *statvfs)
{
	struct stat stat;
	if (fstat(fd, &stat) < 0)
		return -1;

	return fill_statvfs(stat.st_dev, statvfs);
}

