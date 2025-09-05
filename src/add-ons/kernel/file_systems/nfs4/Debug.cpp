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
		kprintf("usage: nfs4_inode [-i] <address(es) ...>\n"
			"  -i specifies that the address is that of an Inode (rather than a VnodeToInode)\n"
			"  address(es):  address of one or more objects of the same type, "
			"separated by spaces\n"
			"  VnodeToInode addresses can be found with the 'vnodes' command.\n"
			"  Output of the 'nfs4' command refers to nodes by their Inode address.\n");
		return 0;
	}

	int argIndex = 1;
	bool dumpVti = true;

	if (strcmp(argv[argIndex], "-i") == 0) {
		dumpVti = false;
		++argIndex;
	}

	for (; argIndex < argc; ++argIndex) {
		if (dumpVti) {
			VnodeToInode* vti = reinterpret_cast<VnodeToInode*>(strtoul(argv[argIndex], NULL, 0));
			if (vti == NULL)
				continue;
			vti->Dump(kprintf);
		} else {
			Inode* inode = reinterpret_cast<Inode*>(strtoul(argv[argIndex], NULL, 0));
			if (inode == NULL)
				continue;
			inode->Dump(kprintf);
		}
		kprintf("\n");
	}

	return 0;
}

