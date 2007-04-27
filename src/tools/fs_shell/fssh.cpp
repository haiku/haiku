/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include <BeOSBuildCompatibility.h>

#include <stdio.h>
#include <string.h>

#include "fssh_errors.h"
#include "fssh_module.h"
#include "module.h"
#include "syscalls.h"
#include "vfs.h"


using namespace FSShell;

namespace FSShell {
	extern fssh_file_system_module_info gRootFileSystem;
}

extern fssh_module_info *modules[];


static fssh_status_t
init_kernel()
{
	fssh_status_t error;

	// init module subsystem
	error = module_init(NULL);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "module_init() failed: %s\n", strerror(error));
		return error;
	} else
		printf("module_init() succeeded!\n");

	// register built-in modules, i.e. the rootfs and the client FS
	register_builtin_module(&gRootFileSystem.info);
	for (int i = 0; modules[i]; i++)
		register_builtin_module(modules[i]);

	// init VFS
	error = vfs_init(NULL);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "initializing VFS failed: %s\n", strerror(error));
		return error;
	} else
		printf("VFS initialized successfully!\n");

	// mount root FS
	fssh_dev_t rootDev = _kern_mount("/", NULL, "rootfs", 0, NULL, 0);
	if (rootDev < 0) {
		fprintf(stderr, "mounting rootfs failed: %s\n", strerror(rootDev));
		return rootDev;
	} else
		printf("mounted rootfs successfully!\n");

	// create mount point for the client FS
	error = _kern_create_dir(-1, "/myfs", 0775);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "creating mount point failed: %s\n", strerror(error));
		return error;
	} else
		printf("mount point created successfully!\n");

	return FSSH_B_OK;
}


int
main(int argc, const char* const* argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: ...\n");
		return 1;
	}

	const char* device = argv[1];

	// get FS module
	if (!modules[0]) {
		fprintf(stderr, "Couldn't find FS module!\n");
		return 1;
	}
	const char* fsName = modules[0]->name;

	fssh_status_t error;

	// init kernel
	error = init_kernel();
	if (error != FSSH_B_OK) {
		fprintf(stderr, "initializing kernel failed: %s\n", strerror(error));
		return error;
	} else
		printf("kernel initialized successfully!\n");

	// mount FS
	fssh_dev_t fsDev = _kern_mount("/myfs", device, fsName, 0, NULL, 0);
	if (fsDev < 0) {
		fprintf(stderr, "mounting FS failed: %s\n", strerror(fsDev));
		return 1;
	} else
		printf("mounted FS successfully!\n");

	// unmount FS
	error = _kern_unmount("/myfs", 0);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "unmounting FS failed: %s\n", strerror(error));
		return error;
	} else
		printf("FS unmounted successfully!\n");

	return 0;
}
