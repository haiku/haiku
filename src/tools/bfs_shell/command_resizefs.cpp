/*
 * Copyright 2012, Andreas Henriksson, sausageboy@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "fssh_stdio.h"
#include "syscalls.h"

#include "bfs.h"
#include "bfs_control.h"


namespace FSShell {


fssh_status_t
command_resizefs(int argc, const char* const* argv)
{
	if (argc != 2) {
		fssh_dprintf("Usage: %s <new size>\n", argv[0]);
		return B_ERROR;
	}

	uint64 newSize;
	if (fssh_sscanf(argv[1], "%" B_SCNu64, &newSize) < 1) {
		fssh_dprintf("Unknown argument or invalid size\n");
		return B_ERROR;
	}

	int rootDir = _kern_open_dir(-1, "/myfs");
	if (rootDir < 0) {
		fssh_dprintf("Error: Couldn't open root directory\n");
		return rootDir;
	}

	status_t status = _kern_ioctl(rootDir, BFS_IOCTL_RESIZE,
		&newSize, sizeof(newSize));

	_kern_close(rootDir);

	if (status != B_OK) {
		fssh_dprintf("Resizing failed, status: %s\n", fssh_strerror(status));
		return status;
	}

	fssh_dprintf("File system successfully resized!\n");
	return B_OK;
}


}	// namespace FSShell
