/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/partitions.h>
#include <boot/vfs.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>


//	#pragma mark -


static status_t
bfs_get_file_system(Node *device, Directory **_root)
{
	int fd = open_node(device, O_RDONLY);
	if (fd >= 0)
		close(fd);

	return B_ERROR;
}


file_system_module_info gBFSFileSystemModule = {
	kPartitionTypeBFS,
	bfs_get_file_system
};

