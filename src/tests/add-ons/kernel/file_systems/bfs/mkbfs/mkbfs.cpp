/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Volume.h"

#include "compat.h"
#include "kprotos.h"

#include <string.h>
#include <stdio.h>


int
main(int argc, char **argv)
{
	char *deviceName = "/boot/home/test-device";
	uint32 blockSize = 1024;

	// ToDo: evaluate arguments!

    init_block_cache(16348, 0);
    init_vnode_layer();

	Volume volume(42);
	status_t status = volume.Initialize(deviceName, "New Disk", blockSize, 0);
	if (status < B_OK) {
		fprintf(stderr, "Initializing volume failed: %s\n", strerror(status));
		return -1;
	}

	shutdown_block_cache();
	return 0;
}

