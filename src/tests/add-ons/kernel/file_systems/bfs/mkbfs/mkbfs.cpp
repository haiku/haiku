/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Volume.h"

#include "compat.h"
#include "kprotos.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>


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

	Volume volume(1);
	status_t status = volume.Initialize(deviceName, volumeName, blockSize, flags);
	if (status < B_OK) {
		fprintf(stderr, "%s: Initializing volume failed: %s\n", programName, strerror(status));
		return -1;
	}

	if (verbose) {
		// ToDo: dump disk info
	}

	shutdown_block_cache();
	sync();

	return 0;
}

