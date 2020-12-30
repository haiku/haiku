/*
 * Copyright 2020-2021 Haiku, Inc. All rights reserved.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define EXIT_FAILURE 1


//#define DEBUG_MBRTOOL
#ifdef DEBUG_MBRTOOL
#   define TRACE(x...) printf("mbrtool: " x)
#else
#   define TRACE(x...) ;
#endif

#define INFO(x...) printf("mbrtool: " x)


// Disk sector size, 512 assumed!
static const size_t kSectorSize = 512;

static void
print_usage(bool error)
{
	printf("\n");
	printf("usage: mbrtool (options) <diskImage> <id> <type> <start> <len>\n");
	printf("       <diskImage>            Disk image to operate on\n");
	printf("       <id>                   Partition ID (0-3)\n");
	printf("       <type>                 Partition type id (hex)\n");
	printf("       <start>                Partition start offset (KiB)\n");
	printf("       <len>                  Partition length (KiB)\n\n");
	printf("  Options:\n");
	printf("       -a, --active           Partition boot flag\n");
	printf("\nWarning: This tool requires precision!\n");
	printf("         Inputs are only lightly validated!\n\n");
	exit(error ? EXIT_FAILURE : 0);
}


static void
checkError(bool failed, const char *message)
{
	if (!failed)
		return;

	if (errno != 0)
		INFO("Error: %s: %s\n", message, strerror(errno));
	else
		INFO("Error: %s\n", message);

	exit(1);
}


static ssize_t
mbrWipe(int handle)
{
	// Blow away MBR while avoiding bootloader
	uint8_t emptyMBR[66] = {};
	// End of MBR marker
	emptyMBR[64] = 0x55;
	emptyMBR[65] = 0xAA;
	return pwrite(handle, emptyMBR, 66, 0x1BE);
}


static bool
mbrValid(int handle)
{
	// TODO: this is a really basic check and ignores invalid
	// partition table entries
	uint8_t mbrBytes[66] = {};
	ssize_t read = pread(handle, mbrBytes, 66, 0x1BE);
	checkError(read < 0, "failed to read MBR for validation");
	return (mbrBytes[64] == 0x55 && mbrBytes[65] == 0xAA);
}


static void
createPartition(int handle, int index, bool active, uint8_t type,
	uint64_t offset, uint64_t size)
{
	uint8_t bootable = active ? 0x80 : 0x0;
	uint8_t partition[16] = {
		bootable,		// bootable
		0xff, 0xff, 0xff,	// CHS first block (default to LBA)
		type,			// partition type
		0xff, 0xff, 0xff,	// CHS last block (default to LBA)
		0x00, 0x00, 0x00, 0x00,	// imageOffset in blocks (written below)
		0x00, 0x00, 0x00, 0x00	// imageSize in blocks (written below)
	};

	// fill in LBA values
	uint32_t partitionOffset = (uint32_t)(offset / kSectorSize);
	((uint32_t *)partition)[2] = partitionOffset;
	((uint32_t *)partition)[3] = (uint32_t)(size / kSectorSize);

	TRACE("%s: #%d %c bytes: %u-%u, sectors: %u-%u\n", __func__, index,
		active ? 'b' : '-', offset, offset + size, partitionOffset,
		partitionOffset + uint32_t(size / kSectorSize));

	ssize_t written = pwrite(handle, partition, 16, 512 - 2 - 16 * (4 - index));
	checkError(written != 16, "failed to write partition entry");

	if (active) {
		// make it bootable
		written = pwrite(handle, &partitionOffset, 4, offset + 512 - 2 - 4);
		checkError(written != 4, "failed to make partition bootable");
	}
	return;
}


int
main(int argc, char *argv[])
{
	const char *imageFile = NULL;

	int partType = -1;
	int partIndex = -1;
	int64_t partStartOffset = -1;
	int64_t partLength = -1;
	bool partBootable = false;

	while (1) {
		int c;
		static struct option long_options[] = {
			{"active", no_argument, 0, 'a'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

		opterr = 1; /* don't print errors */
		c = getopt_long(argc, argv, "+ha", long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 's':
				print_usage(false);
				break;
			case 'a':
				partBootable = true;
				break;
			default:
				print_usage(true);
		}
	}

	if ((argc - optind) != 5)
		print_usage(true);

	for (int index = optind; index < argc; index++) {
		if (imageFile == NULL)
			imageFile = argv[index];
		else if (partIndex < 0)
			partIndex = atoi(argv[index]);
		else if (partType < 0)
			partType = (int)strtol(argv[index], NULL, 0);
		else if (partStartOffset < 0 ) {
			partStartOffset = (int64_t)strtol(argv[index], NULL, 10);
			partStartOffset *= 1024;
		} else if (partLength < 0) {
			partLength = (int64_t)strtol(argv[index], NULL, 10);
			partLength *= 1024;
		}
	}

	checkError(partIndex < 0 || partIndex > 3,
		"invalid partition index, valid range is 0-3");
	checkError(partType < 0,  "Incorrect Partition Type!");

	checkError((partStartOffset + partLength) > 2089072000000,
		"partitions beyond 2TiB are not accepted!");

	int imageFileHandle = open(imageFile, O_RDWR);
	checkError(imageFileHandle < 0, "failed to open disk image file");

	struct stat stat;
	int result = fstat(imageFileHandle, &stat);
	checkError(result != 0, "failed to stat image file");
	off_t imageSize = stat.st_size;

	if (!mbrValid(imageFileHandle)) {
		INFO("MBR of image is invalid, creating a fresh one.\n");
		mbrWipe(imageFileHandle);
	}

	// Just a warning. This is technically valid since MBR partition
	// definitions are purely within the first 512 bytes
	if (partStartOffset + partLength > imageSize)
		INFO("Warning: Partition extends beyond end of file!\n");

	createPartition(imageFileHandle, partIndex, partBootable, partType,
		partStartOffset, partLength);
	return 0;
}
