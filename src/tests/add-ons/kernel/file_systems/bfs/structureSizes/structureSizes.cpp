/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <SupportDefs.h>

#undef _PACKED
#define _PACKED

#include "bfs.h"

#include <stdio.h>
#include <stddef.h>


#define SIZEOF(x) "%20s = %lu\n", #x, sizeof(x)
#define OFFSETOF(x, y) #x "." #y " at offset %lu\n", offsetof(x, y)


int 
main(int argc, char **argv)
{
	printf(SIZEOF(disk_super_block));
	printf(SIZEOF(small_data));
	printf(SIZEOF(data_stream));
	printf(SIZEOF(bfs_inode));

	puts("");

	printf(OFFSETOF(bfs_inode, inode_size));
	printf(OFFSETOF(bfs_inode, etc));
	printf(OFFSETOF(bfs_inode, data));

	return 0;
}
