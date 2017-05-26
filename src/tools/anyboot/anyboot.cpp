/*
 * Copyright 2010 Michael Lotz, mmlr@mlotz.ch
 * Copyright 2011-2017 Haiku, Inc. All rights reserved.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Michael Lotz <mmlr@mlotz.ch>
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


//#define DEBUG_ANYBOOT
#ifdef DEBUG_ANYBOOT
#   define TRACE(x...) printf("anyboot: " x)
#else
#   define TRACE(x...) ;
#endif


static uint8_t *sCopyBuffer = NULL;
static size_t sCopyBufferSize = 2 * 1024 * 1024;
static const size_t kBlockSize = 512;


// Haiku Anyboot Image:
//   (MBR Table + Boot Sector)
//   ISO (Small Haiku ISO9660)
//   First Partition (Haiku OS Image, BFS)
//   Second Partition (EFI Loader, FAT)
//   Third Partition (EFI Mac, HFS) <not implemented>


static void
print_usage(bool error)
{
	printf("\n");
	printf("usage: anyboot [-b BIOS-Loader] [-e EFI-Filesystem] <isoFile> <imageFile> <outputFile>\n");
	printf("       -b, --bios-loader <file>     Legacy BIOS bootloader\n");
	printf("       -e, --efi-filesystem <file>  EFI filesystem\n");
	exit(error ? EXIT_FAILURE : 0);
}


static void
checkError(bool failed, const char *message)
{
	if (!failed)
		return;

	printf("%s: %s\n", message, strerror(errno));
	free(sCopyBuffer);
	exit(1);
}


#if 0
static void
chsAddressFor(uint32_t offset, uint8_t *address, uint32_t sectorsPerTrack,
	uint32_t headsPerCylinder)
{
	if (offset >= 1024 * sectorsPerTrack * headsPerCylinder) {
		// not encodable, force LBA
		address[0] = 0xff;
		address[1] = 0xff;
		address[2] = 0xff;
		return;
	}

	uint32_t cylinders = 0;
	uint32_t heads = 0;
	uint32_t sectors = 0;

	uint32_t temp = 0;
	while (temp * sectorsPerTrack * headsPerCylinder <= offset)
		cylinders = temp++;

	offset -= (sectorsPerTrack * headsPerCylinder * cylinders);

	temp = 0;
	while (temp * sectorsPerTrack <= offset)
		heads = temp++;

	sectors = offset - (sectorsPerTrack * heads) + 1;

	address[0] = heads;
	address[1] = sectors;
	address[1] |= (cylinders >> 2) & 0xc0;
	address[2] = cylinders;
}
#endif


static void
createPartition(int handle, int index, bool active, uint8_t type,
	uint32_t offset, uint32_t size)
{
	uint8_t bootable = active ? 0x80 : 0x0;
	uint8_t partition[16] = {
		bootable,				// bootable
		0xff, 0xff, 0xff,		// CHS first block (default to LBA)
		type,					// partition type
		0xff, 0xff, 0xff,		// CHS last block (default to LBA)
		0x00, 0x00, 0x00, 0x00,	// imageOffset in blocks (written below)
		0x00, 0x00, 0x00, 0x00	// imageSize in blocks (written below)
	};

	// fill in LBA values
	uint32_t partitionOffset = (uint32_t)(offset / kBlockSize);
	((uint32_t *)partition)[2] = partitionOffset;
	((uint32_t *)partition)[3] = (uint32_t)(size / kBlockSize);

	TRACE("%s: #%d %c bytes: %u-%u, sectors: %u-%u\n", __func__, index,
		active ? 'b' : '-', offset, offset + size, partitionOffset,
		partitionOffset + uint32_t(size / kBlockSize));
#if 0
	// while this should basically work, it makes the boot code needlessly
	// use chs which has a high potential of failure due to the geometry
	// being unknown beforehand (a fixed geometry would be used here).

	uint32_t sectorsPerTrack = 63;
	uint32_t headsPerCylinder = 255;

	// fill in CHS values
	chsAddressFor(partitionOffset, &partition[1], sectorsPerTrack,
		headsPerCylinder);
	chsAddressFor(partitionOffset + (uint32_t)(size / kBlockSize),
		&partition[5], sectorsPerTrack, headsPerCylinder);
#endif

	ssize_t written = pwrite(handle, partition, 16, 512 - 2 - 16 * (4 - index));
	checkError(written != 16, "failed to write partition entry");

	if (active) {
		// make it bootable
		written = pwrite(handle, &partitionOffset, 4, offset + 512 - 2 - 4);
		checkError(written != 4, "failed to make image bootable");
	}
	return;
}


static int
copyLoop(int from, int to, off_t position)
{
	while (true) {
		ssize_t copyLength = read(from, sCopyBuffer, sCopyBufferSize);
		if (copyLength <= 0)
			return copyLength;

		ssize_t written = pwrite(to, sCopyBuffer, copyLength, position);
		if (written != copyLength) {
			if (written < 0)
				return written;
			else
				return -1;
		}

		position += copyLength;
	}
}


int
main(int argc, char *argv[])
{
	const char *biosFile = NULL;
	const char *efiFile = NULL;
	const char *isoFile = NULL;
	const char *imageFile = NULL;
	const char *outputFile = NULL;

	while (1) {
		int c;
		static struct option long_options[] = {
			{"bios-loader", required_argument, 0, 'b'},
			{"efi-loader", required_argument, 0, 'e'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

		opterr = 1; /* don't print errors */
		c = getopt_long(argc, argv, "+hb:e:", long_options, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage(false);
				break;
			case 'b':
				biosFile = optarg;
				break;
			case 'e':
				efiFile = optarg;
				break;
			default:
				print_usage(true);
		}
	}

	if ((argc - optind) != 3)
		print_usage(true);

	for (int index = optind; index < argc; index++) {
		if (isoFile == NULL)
			isoFile = argv[index];
		else if (imageFile == NULL)
			imageFile = argv[index];
		else if (outputFile == NULL)
			outputFile = argv[index];
	}

	sCopyBuffer = (uint8_t *)malloc(sCopyBufferSize);
	checkError(sCopyBuffer == NULL, "no memory for copy buffer");

	int outputFileHandle = open(outputFile, O_WRONLY | O_TRUNC | O_CREAT,
		S_IRUSR | S_IWUSR);
	checkError(outputFileHandle < 0, "failed to open output file");

	int isoFileHandle = open(isoFile, O_RDONLY);
	checkError(isoFileHandle < 0, "failed to open ISO file");

	struct stat stat;
	int result = fstat(isoFileHandle, &stat);
	checkError(result != 0, "failed to stat ISO file");
	off_t isoSize = stat.st_size;

	int biosFileHandle = -1;
	if (biosFile != NULL) {
		biosFileHandle = open(biosFile, O_RDONLY);
		checkError(biosFileHandle < 0, "failed to open BIOS bootloader file");
	}

	int efiFileHandle = -1;
	off_t efiSize = 0;
	if (efiFile != NULL) {
		efiFileHandle = open(efiFile, O_RDONLY);
		checkError(efiFileHandle < 0, "failed to open EFI bootloader file");

		result = fstat(efiFileHandle, &stat);
		checkError(result != 0, "failed to stat EFI filesystem image");
		efiSize = stat.st_size;
	}

	int imageFileHandle = open(imageFile, O_RDONLY);
	checkError(imageFileHandle < 0, "failed to open image file");

	result = fstat(imageFileHandle, &stat);
	checkError(result != 0, "failed to stat image file");
	off_t imageSize = stat.st_size;

	result = copyLoop(isoFileHandle, outputFileHandle, 0);
	checkError(result != 0, "failed to copy iso file to output");

	// isoSize rounded to the next full megabyte
	off_t alignment = 1 * 1024 * 1024 - 1;
	off_t imageOffset = (isoSize + alignment) & ~alignment;

	result = copyLoop(imageFileHandle, outputFileHandle, imageOffset);
	checkError(result != 0, "failed to copy image file to output");

	if (biosFileHandle >= 0) {
		result = copyLoop(biosFileHandle, outputFileHandle, 0);
		checkError(result != 0, "failed to copy BIOS bootloader to output");
	}

	// Haiku Image Partition
	alignment = kBlockSize - 1;
	imageSize = (imageSize + alignment) & ~alignment;
	createPartition(outputFileHandle, 0, true, 0xeb, imageOffset, imageSize);

	// Optional EFI Filesystem
	if (efiFile != NULL) {
		off_t efiOffset = (imageOffset + imageSize + alignment) & ~alignment;
		efiSize = (efiSize + alignment) & ~alignment;
		result = copyLoop(efiFileHandle, outputFileHandle, efiOffset);
		checkError(result != 0, "failed to copy EFI filesystem image to output");
		createPartition(outputFileHandle, 1, false, 0xef, efiOffset, efiSize);
	}

	// TODO: MacEFI (HFS) maybe someday

	free(sCopyBuffer);
	return 0;
}
