
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	int8	int8_t
#define	int16	int16_t
#define	int32	int32_t
#define	int64	int64_t
#define	uint8	uint8_t
#define	uint16	uint16_t
#define	uint32	uint32_t
#define	uint64	uint64_t

typedef int64 bfs_off_t;

struct block_run {
	int32		allocation_group;
	uint16		start;
	uint16		length;
};

typedef block_run inode_addr;

#define BFS_DISK_NAME_LENGTH	32

struct disk_super_block {
	char		name[BFS_DISK_NAME_LENGTH];
	int32		magic1;
	int32		fs_byte_order;
	uint32		block_size;
	uint32		block_shift;
	bfs_off_t	num_blocks;
	bfs_off_t	used_blocks;
	int32		inode_size;
	int32		magic2;
	int32		blocks_per_ag;
	int32		ag_shift;
	int32		num_ags;
	int32		flags;
	block_run	log_blocks;
	bfs_off_t	log_start;
	bfs_off_t	log_end;
	int32		magic3;
	inode_addr	root_dir;
	inode_addr	indices;
	int32		pad[8];
};

#define SUPER_BLOCK_MAGIC1			'BFS1'		/* BFS1 */

const char *kHexDigits = "0123456789abcdef";


static void
read_from(int fd, off_t pos, void *buffer, size_t size)
{
	if (lseek(fd, pos, SEEK_SET) < 0
		|| read(fd, buffer, size) != (int)size) {
		fprintf(stderr, "Error: Failed to read %lu bytes at offset %lld\n",
			size, pos);
		exit(1);
	}
}


static void
write_to(int fd, off_t pos, const void *buffer, size_t size)
{
	if (lseek(fd, pos, SEEK_SET) < 0
		|| write(fd, buffer, size) != (int)size) {
		fprintf(stderr, "Error: Failed to write %lu bytes at offset %lld\n",
			size, pos);
		exit(1);
	}
}

static int
count_bits(int value)
{
	int count = 0;

	while (value) {
		if (value & 1)
			count++;
		value >>= 1;
	}

	return count;
}

int
main(int argc, const char *const *argv)
{
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <image> [ <hex OR pattern> ]\n", argv[0]);
		exit(1);
	}

	const char *fileName = argv[1];
	const char *patternString = (argc >= 3 ? argv[2] : "0f");

	// skip leading "0x" in the pattern string
	if (strncmp(patternString, "0x", 2) == 0)
		patternString += 2;

	// translate the pattern
	uint8 *pattern = new uint8[strlen(patternString) / 2 + 1];
	int patternLen = 0;
	for (; *patternString; patternString++) {
		if (!isspace(*patternString)) {
			char c = *patternString;
			const char *digitPos = strchr(kHexDigits, tolower(c));
			if (!digitPos) {
				fprintf(stderr, "Error: Invalid pattern character: \"%c\"\n",
					c);
				exit(1);
			}
			int digit = digitPos - kHexDigits;

			if (patternLen & 1)
				pattern[patternLen / 2] |= digit;
			else
				pattern[patternLen / 2] = digit << 4;
			patternLen++;
		}
	}

	if (patternLen == 0 || (patternLen & 1)) {
		fprintf(stderr, "Error: Invalid pattern! Must have even number (>= 2) "
			"of hex digits.\n");
		exit(1);
	}
	patternLen /= 2;	// is now length in bytes

	printf("bfs_fragmenter: using pattern: 0x");
	for (int i = 0; i < patternLen; i++)
		printf("%02x", pattern[i]);
	printf("\n");

	// open file
	int fd = open(fileName, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error: Failed to open \"%s\": %s\n", fileName,
			strerror(errno));
		exit(1);
	}

	// read superblock
	disk_super_block superBlock;
	read_from(fd, 512, &superBlock, sizeof(superBlock));

	if (superBlock.magic1 != SUPER_BLOCK_MAGIC1) {
		fprintf(stderr, "Error: Bad superblock magic. This is probably not a "
			"BFS image.\n");
		exit(1);
	}
	int blockSize = superBlock.block_size;

	// stat the image
	struct stat st;
	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "Error: Failed to stat image: %s\n", strerror(errno));
		exit(1);
	}

	int64 blockCount = st.st_size / blockSize;
	if (blockCount != superBlock.num_blocks) {
		fprintf(stderr, "Error: Number of blocks in image and the number in "
			"the superblock differ: %lld vs. %lld\n", blockCount,
			superBlock.num_blocks);
		exit(1);
	}


	// iterate through the block bitmap blocks and or the bytes with 0x0f
	uint8 *block = new uint8[blockSize];
	int64 occupiedBlocks = 0;
	int64 bitmapByteCount = blockCount / 8;		// round down, we ignore the
												// last partial byte
	int64 bitmapBlockCount = (bitmapByteCount + blockSize - 1) / blockSize;

	int bitmapByteIndex = 0;
	for (int i = 1; i < bitmapBlockCount + 1; i++) {
		off_t offset = i * blockSize;
		read_from(fd, offset, block, blockSize);

		int toProcess = blockSize;
		if (toProcess > bitmapByteCount - bitmapByteIndex)
			toProcess = (bitmapByteCount - bitmapByteIndex);

		for (int k = 0; k < toProcess; k++) {
			uint8 patternChar = pattern[bitmapByteIndex % patternLen];
			occupiedBlocks += count_bits(~block[k] & patternChar);
			block[k] |= patternChar;
			bitmapByteIndex++;
		}

		write_to(fd, offset, block, blockSize);
	}

	printf("bfs_fragmenter: marked %lld blocks used\n", occupiedBlocks);

	// write back the superblock
	superBlock.used_blocks += occupiedBlocks;
	write_to(fd, 512, &superBlock, sizeof(superBlock));

	return 0;
}

