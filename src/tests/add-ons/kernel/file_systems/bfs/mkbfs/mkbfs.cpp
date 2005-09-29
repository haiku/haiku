/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include "boot_block.h"
#include "compat.h"

// the definitions in fsproto.h (included by kprotos.h) and fs_interface.h
// (included by Volume.h) are not compatible
#define get_vnode _get_vnode
#define put_vnode _put_vnode
#define new_vnode _new_vnode
#define remove_vnode _remove_vnode
#define unremove_vnode _unremove_vnode
#define notify_listener _notify_listener
#define send_notification _send_notification

#include "kprotos.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>


// Note: these structures have been stolen from fs_shell/kernel.c.
//		They shouldn't be used anywhere else

extern vnode_ops fs_entry;
struct vnode;

struct vnlist {
	vnode           *head;
	vnode           *tail;
	int             num;
};

struct nspace {
	nspace_id           nsid;
	my_dev_t            dev;
	my_ino_t            ino;
	fsystem             *fs;
	vnlist              vnodes;
	void                *data;
	vnode				*root;
	vnode				*mount;
	nspace				*prev;
	nspace				*next;
	char                shutdown;
};


void
usage(char *programName)
{
	fprintf(stderr, "usage: %s [-vn] [-b block-size] device-path volume-name\n"
		"Creates a new BFS file system on the specified device (or image)\n"
		"\t-b\tblock size (defaults to 1024)\n"
		"\t-n\tdisable indices (long option --noindex)\n"
		"\t-v\tfor verbose output.\n",
		programName);

	exit(0);
}


int
main(int argc, char **argv)
{
	char *programName = argv[0];
	if (strrchr(programName, '/'))
		programName = strrchr(programName, '/') + 1;

	if (argc < 2)
		usage(programName);

	uint32 blockSize = 1024;
	uint32 flags = 0;
	bool verbose = false;

	while (*++argv) {
		char *arg = *argv;
		if (*arg == '-') {
			if (arg[1] == '-') {
				if (!strcmp(arg, "--noindex"))
					flags |= VOLUME_NO_INDICES;
				else
					usage(programName);
			}

			while (*++arg && isalpha(*arg)) {
				switch (*arg) {
					case 'v':
						verbose = true;
						break;
					case 'b':
						if (*++argv == NULL || !isdigit(**argv))
							usage(programName);

						blockSize = atol(*argv);
						break;
					case 'n':
						flags |= VOLUME_NO_INDICES;
						break;
				}
			}
		}
		else
			break;
	}

	char *deviceName = argv[0];
	if (deviceName == NULL) {
		fprintf(stderr, "%s: You must at least specify a device where "
			"the image is going to be created\n", programName);
		return -1;
	}

	char *volumeName = "Unnamed";
	if (argv[1] != NULL)
		volumeName = argv[1];

	if (blockSize != 1024 && blockSize != 2048 && blockSize != 4096 && blockSize != 8192) {
		fprintf(stderr, "%s: valid block sizes are: 1024, 2048, 4096, and 8192\n", programName);
		return -1;
	}

	// set up tiny VFS and initialize the device/image

    init_block_cache(4096, 0);
    init_vnode_layer();

    if (install_file_system(&fs_entry, "bfs", 1, -1) == NULL) {
        printf("can't install my file system\n");
        exit(0);
    }

	struct nspace *mount = (nspace *)malloc(sizeof(nspace));
	if (add_nspace(mount, NULL, "bfs", -1, -1) < B_OK) {
		fprintf(stderr, "%s: add mount structure failed\n", programName);
		return -1;
	}

	Volume volume(mount->nsid);
	status_t status = volume.Initialize(deviceName, volumeName, blockSize, flags);
	if (status < B_OK) {
		fprintf(stderr, "%s: Initializing volume failed: %s\n", programName, strerror(status));
		return -1;
	}

	remove_nspace(mount);

	if (verbose) {
		disk_super_block super = volume.SuperBlock();

		printf("%s: Disk was initialized successfully.\n", programName);
		printf("\tname: \"%s\"\n", super.name);
		printf("\tnum blocks: %Ld\n", super.NumBlocks());
		printf("\tused blocks: %Ld\n", super.UsedBlocks());
		printf("\tblock size: %lu bytes\n", super.BlockSize());
		printf("\tnum allocation groups: %ld\n", super.AllocationGroups());
		printf("\tallocation group size: %ld blocks\n", 1L << super.AllocationGroupShift());
		printf("\tlog size: %u blocks\n", super.log_blocks.Length());
	}

	shutdown_block_cache();
	
	// make the disk image bootable

	int device = open(deviceName, O_RDWR);
	if (device < 0) {
		fprintf(stderr, "%s: Could not make image bootable: %s\n", programName, strerror(errno));
		return -1;
	}

	// change BIOS drive and partition offset
	// ToDo: for now, this will only work for images only

	sBootBlockData1[kBIOSDriveOffset] = 0x80;
		// for now, this should be replaced by the real thing
	uint32 *offset = (uint32 *)&sBootBlockData1[kPartitionDataOffset];
	*offset = 0;

	write_pos(device, 0, sBootBlockData1, kBootBlockData1Size);
	write_pos(device, kBootBlockData2Offset, sBootBlockData2, kBootBlockData2Size);

	close(device);
	sync();

	return 0;
}

