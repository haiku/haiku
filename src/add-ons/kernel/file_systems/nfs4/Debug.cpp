/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Debug.h"
#include "FileSystem.h"
#include "VnodeToInode.h"


int
kprintf_volume(int argc, char** argv)
{
	if ((argc == 1) || strcmp(argv[1], "--help") == 0) {
		kprintf("usage: nfs4 <address>\n"
			"  address:  address of a nfs4 private volume (FileSystem)\n"
			"  Use 'mounts' to list mounted volume ids, and 'mount <id>' to display a private "
			"volume address.\n");
		return 0;
	}

	FileSystem* volume = reinterpret_cast<FileSystem*>(strtoul(argv[1], NULL, 0));
	volume->Dump(kprintf);

	return 0;
}


int
kprintf_inode(int argc, char** argv)
{
	if ((argc == 1) || strcmp(argv[1], "--help") == 0) {
		kprintf("usage: nfs4_inode <address(es) ...>\n"
			"  address(es):  address of one or more nfs4 private nodes (VnodeToInode), "
			"separated by spaces\n"
			"  Addresses can be found with the 'vnodes' command.\n");
		return 0;
	}

	for (int i = 1; i < argc; i++) {
		VnodeToInode* node = reinterpret_cast<VnodeToInode*>(strtoul(argv[1], NULL, 0));
		if (node == NULL)
			continue;
		node->Dump(kprintf);
		kprintf("----------\n");
	}

	return 0;
}
