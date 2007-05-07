/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


static const uint32 kDesiredAllocationGroups = 56;


int
main(int argc, char **argv)
{
	if (argc != 3 || !isdigit(argv[1][0]) || !isdigit(argv[2][0])) {
		const char *name = strrchr(argv[0], '/');
		name = name ? name + 1 : argv[0];

		fprintf(stderr, "usage: %s <num blocks> <block size>\n", name);
		return -1;
	}

	off_t numBlocks = atoll(argv[1]);
	uint32 blockSize = (uint32)atol(argv[2]);

	if (blockSize != 1024 && blockSize != 2048 && blockSize != 4096 && blockSize != 8192) {
		fprintf(stderr, "valid values for <block size> are: 1024, 2048, 4096, 8192\n");
		return -1;
	}

	int32 blockShift = 9;
	while ((1UL << blockShift) < blockSize) {
		blockShift++;
	}

	uint32 blocks_per_ag = 1;
	uint32 ag_shift = 13;
	uint32 num_ags = 0;

	uint32 bitsPerBlock = blockSize << 3;
	off_t bitmapBlocks = (numBlocks + bitsPerBlock - 1) / bitsPerBlock;
	for (uint32 i = 8192; i < bitsPerBlock; i *= 2) {
		ag_shift++;
	}

	// Many allocation groups help applying allocation policies, but if
	// they are too small, we will need to many block_runs to cover large
	// files

	while (true) {
		num_ags = (bitmapBlocks + blocks_per_ag - 1) / blocks_per_ag;
		if (num_ags > kDesiredAllocationGroups) {
			if (ag_shift == 16)
				break;

			ag_shift++;
			blocks_per_ag *= 2;
		} else
			break;
	}

	printf("blocks = %Ld\n", numBlocks);
	printf("block shift = %ld\n", blockShift);
	printf("bits per block = %lu\n", bitsPerBlock);
	printf("bitmap blocks = %Ld\n", bitmapBlocks);
	printf("allocation groups = %lu\n", num_ags);
	printf("shift = %lu (%lu)\n", ag_shift, 1UL << ag_shift);
	printf("blocks per group = %lu\n", blocks_per_ag);

	uint32 bitsPerGroup = 8 * (blocks_per_ag << blockShift);

	printf("\nBits per group: %ld\n", bitsPerGroup);
	printf("Bits in last group: %ld\n", numBlocks - (num_ags - 1) * bitsPerGroup);

	return 0;
}

