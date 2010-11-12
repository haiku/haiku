/*
 * Copyright 2010 Michael Lotz, mmlr@mlotz.ch
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static uint8_t *sCopyBuffer = NULL;
static size_t sCopyBufferSize = 2 * 1024 * 1024;


int
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


void
checkError(bool failed, const char *message)
{
	if (!failed)
		return;

	printf("%s: %s\n", message, strerror(errno));
	free(sCopyBuffer);
	exit(1);
}


void
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


int
main(int argc, char *argv[])
{
	if (argc < 5) {
		printf("usage: %s <outputFile> <isoFile> <mbrFile> <imageFile>\n",
			argv[0]);
		return 1;
	}

	sCopyBuffer = (uint8_t *)malloc(sCopyBufferSize);
	checkError(sCopyBuffer == NULL, "no memory for copy buffer");

	static const size_t kBlockSize = 512;

	int outputFile = open(argv[1], O_WRONLY | O_TRUNC | O_CREAT,
		S_IRUSR | S_IWUSR);
	checkError(outputFile < 0, "failed to open output file");

	int isoFile = open(argv[2], O_RDONLY);
	checkError(isoFile < 0, "failed to open ISO file");

	struct stat stat;
	int result = fstat(isoFile, &stat);
	checkError(result != 0, "failed to stat ISO file");
	off_t isoSize = stat.st_size;

	int mbrFile = open(argv[3], O_RDONLY);
	checkError(mbrFile < 0, "failed to open MBR file");

	int imageFile = open(argv[4], O_RDONLY);
	checkError(imageFile < 0, "failed to open image file");

	result = fstat(imageFile, &stat);
	checkError(result != 0, "failed to stat image file");

	off_t imageSize = stat.st_size;

	result = copyLoop(isoFile, outputFile, 0);
	checkError(result != 0, "failed to copy iso file to output");

	// isoSize rounded to the next full megabyte
	off_t alignment = 1 * 1024 * 1024 - 1;
	off_t imageOffset = (isoSize + alignment) & ~alignment;

	result = copyLoop(imageFile, outputFile, imageOffset);
	checkError(result != 0, "failed to copy image file to output");

	result = copyLoop(mbrFile, outputFile, 0);
	checkError(result != 0, "failed to copy mbr to output");

	// construct the partition table
	uint8_t partition[16] = {
		0x80,					// active
		0xff, 0xff, 0xff,		// CHS first block (default to LBA)
		0xeb,					// partition type (BFS)
		0xff, 0xff, 0xff,		// CHS last block (default to LBA)
		0x00, 0x00, 0x00, 0x00,	// imageOffset in blocks (written below)
		0x00, 0x00, 0x00, 0x00	// imageSize in blocks (written below)
	};

	alignment = kBlockSize - 1;
	imageSize = (imageSize + alignment) & ~alignment;

	// fill in LBA values
	uint32_t partitionOffset = (uint32_t)(imageOffset / kBlockSize);
	((uint32_t *)partition)[2] = partitionOffset;
	((uint32_t *)partition)[3] = (uint32_t)(imageSize / kBlockSize);

#if 0
	// while this should basically work, it makes the boot code needlessly
	// use chs which has a high potential of failure due to the geometry
	// being unknown beforehand (a fixed geometry would be used here).

	uint32_t sectorsPerTrack = 63;
	uint32_t headsPerCylinder = 255;

	// fill in CHS values
	chsAddressFor(partitionOffset, &partition[1], sectorsPerTrack,
		headsPerCylinder);
	chsAddressFor(partitionOffset + (uint32_t)(imageSize / kBlockSize),
		&partition[5], sectorsPerTrack, headsPerCylinder);
#endif

	ssize_t written = pwrite(outputFile, partition, 16, 512 - 2 - 16 * 4);
	checkError(written != 16, "failed to write partition entry");

	// and make the image bootable
	written = pwrite(outputFile, &partitionOffset, 4,
		imageOffset + 512 - 2 - 4);
	checkError(written != 4, "failed to make image bootable");

	free(sCopyBuffer);
	return 0;
}
