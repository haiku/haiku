/*
  This file contains some glue code that initializes the block cache,
  the vnode layer, mounts the root file system (a simple container)
  and then mounts our file system at the mount point /myfs.  You could
  modify this to mount other file systems or even multiple file systems
  if you wanted.
  
  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/
#include <stdio.h>
#include <stdlib.h>

#include "compat.h"
#include "kprotos.h"
#include "fsproto.h"


extern vnode_ops fs_entry;

void *
init_fs(char *disk_name)
{
    int err;
    void *data = NULL;

    init_block_cache(16348, 0);
    init_vnode_layer();

    err = sys_mkdir(1, -1, "/myfs", 0);

    if (install_file_system(&fs_entry, "myfs", 1, -1) == NULL) {
        printf("can't install my file system\n");
        exit(0);
    }

    data = sys_mount(1, "myfs", -1, "/myfs", disk_name, 0, NULL, 0);
    if (data == NULL) {
        printf("could not mount %s on /myfs\n", disk_name);
        exit(0);
    }

	err = sys_chdir(1, -1, "/myfs");
	if (err != 0) {
		printf("Failed to cd into /myfs.");
		sys_unmount(1, -1, "/myfs");
		exit(0);
	}
    
    return data;
}

status_t
initialize_fs(const char *deviceName, void *params, int paramLength)
{
    status_t error;
	
	init_block_cache(16348, 0);
    init_vnode_layer();

    if (install_file_system(&fs_entry, "myfs", 1, -1) == NULL) {
        fprintf(stderr, "can't install my file system\n");
		return FS_ERROR;
    }

	error = initialize_file_system(deviceName, "myfs", params, paramLength);
	if (error < FS_OK) {
		fprintf(stderr, "Initializing the file system failed: %s\n",
			strerror(error));
		return error;
	}

	shutdown_block_cache();

	return FS_OK;
}
