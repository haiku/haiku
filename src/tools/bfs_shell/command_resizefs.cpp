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
	if (argc > 3 || argc < 2) {
		fssh_dprintf("Usage: %s [-n|--dry-run] <new size>\n", argv[0]);
		return B_ERROR;
	}

	resize_control control;
	control.new_size = 0;
	control.dry_run = false;

	int argIndex = 1;
	if (strcmp(argv[1], "-n") == 0 || strcmp(argv[1], "--dry-run") == 0) {
		control.dry_run = true;
		argIndex++;
	}
	if (fssh_sscanf(argv[argIndex], "%" B_SCNu64, &control.new_size) < 1) {
		fssh_dprintf("Unknown argument or invalid size\n");
		return B_ERROR;
	}

	int rootDir = _kern_open_dir(-1, "/myfs");
	if (rootDir < 0) {
		fssh_dprintf("Error: Couldn't open root directory\n");
		return rootDir;
	}

	status_t status = _kern_ioctl(rootDir, BFS_IOCTL_RESIZE, &control, sizeof(control));

	_kern_close(rootDir);

	if (status != B_OK) {
		fssh_dprintf("Resizing failed, status: %s\n", fssh_strerror(status));
		return status;
	}

	if (control.dry_run)
		fssh_dprintf("File system successfully resized in dry mode; no changes made!\n");
	else
		fssh_dprintf("File system successfully resized!\n");
	return B_OK;
}


}	// namespace FSShell
