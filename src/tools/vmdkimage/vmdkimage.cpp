/*
 * Copyright 2007, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vmdk.h>


#if defined(__BEOS__) && !defined(__HAIKU__)
#define pread(_fd, _buf, _count, _pos) read_pos(_fd, _pos, _buf, _count)
#define realpath(x, y)	NULL
#endif


static void
print_usage()
{
	printf("\n");
	printf("vmdkimage\n");
	printf("\n");
	printf("usage: vmdkimage -i <imagesize> -h <headersize> [-c] [-H] "
		"[-u <uuid>] [-f] <file>\n");
	printf("   or: vmdkimage -d <file>\n");
	printf("       -d, --dump         dumps info for the image file\n");
	printf("       -i, --imagesize    size of raw partition image file\n");
	printf("       -h, --headersize   size of the vmdk header to write\n");
	printf("       -f, --file         the raw partition image file\n");
	printf("       -u, --uuid         UUID for the image instead of a computed "
		"one\n");
	printf("       -c, --clear-image  set the image content to zero\n");
	printf("       -H, --header-only  write only the header\n");
	exit(EXIT_FAILURE);
}


static void
dump_image_info(const char *filename)
{
	int image = open(filename, O_RDONLY);
	if (image < 0) {
		fprintf(stderr, "Error: couldn't open file %s (%s)\n", filename,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	SparseExtentHeader header;
	if (read(image, &header, 512) != 512) {
		fprintf(stderr, "Error: couldn't read header: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (header.magicNumber != VMDK_SPARSE_MAGICNUMBER) {
		fprintf(stderr, "Error: invalid header magic.\n");
		exit(EXIT_FAILURE);
	}

	printf("--------------- Header ---------------\n");
	printf("  version:             %d\n", (int)header.version);
	printf("  flags:               %d\n", (int)header.flags);
	printf("  capacity:            %d\n", (int)header.capacity);
	printf("  grainSize:           %lld\n", (long long)header.grainSize);
	printf("  descriptorOffset:    %lld\n", (long long)header.descriptorOffset);
	printf("  descriptorSize:      %lld\n", (long long)header.descriptorSize);
	printf("  numGTEsPerGT:        %u\n", (unsigned int)header.numGTEsPerGT);
	printf("  rgdOffset:           %lld\n", (long long)header.rgdOffset);
	printf("  gdOffset:            %lld\n", (long long)header.gdOffset);
	printf("  overHead:            %lld\n", (long long)header.overHead);
	printf("  uncleanShutdown:     %s\n",
		header.uncleanShutdown ? "yes" : "no");
	printf("  singleEndLineChar:   '%c'\n", header.singleEndLineChar);
	printf("  nonEndLineChar:      '%c'\n", header.nonEndLineChar);
	printf("  doubleEndLineChar1:  '%c'\n", header.doubleEndLineChar1);
	printf("  doubleEndLineChar2:  '%c'\n", header.doubleEndLineChar2);

	if (header.descriptorOffset != 0) {
		printf("\n--------------- Descriptor ---------------\n");
		size_t descriptorSize = header.descriptorSize * 512 * 2;
		char *descriptor = (char *)malloc(descriptorSize);
		if (descriptor == NULL) {
			fprintf(stderr, "Error: cannot allocate descriptor size %u.\n",
				(unsigned int)descriptorSize);
			exit(EXIT_FAILURE);
		}

		if (pread(image, descriptor, descriptorSize,
				header.descriptorOffset * 512) != (ssize_t)descriptorSize) {
			fprintf(stderr, "Error: couldn't read header: %s\n",
				strerror(errno));
			exit(EXIT_FAILURE);
		}

		puts(descriptor);
		putchar('\n');
		free(descriptor);
	}

	close(image);
}


static uint64_t
hash_string(const char *string)
{
	uint64_t hash = 0;
	char c;

	while ((c = *string++) != 0) {
		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}


static bool
is_valid_uuid(const char *uuid)
{
	const char *kHex = "0123456789abcdef";
	for (int i = 0; i < 36; i++) {
		if (!uuid[i])
			return false;
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			if (uuid[i] != '-')
				return false;
			continue;
		}
		if (strchr(kHex, uuid[i]) == NULL)
			return false;
	}

	return uuid[36] == '\0';
}


int
main(int argc, char *argv[])
{
	uint64_t headerSize = 0;
	uint64_t imageSize = 0;
	const char *file = NULL;
	const char *uuid = NULL;
	bool headerOnly = false;
	bool clearImage = false;
	bool dumpOnly = false;

	if (sizeof(SparseExtentHeader) != 512) {
		fprintf(stderr, "compilation error: struct size is %u byte\n",
			(unsigned)sizeof(SparseExtentHeader));
		exit(EXIT_FAILURE);
	}

	while (1) {
		int c;
		static struct option long_options[] = {
			{"dump", no_argument, 0, 'd'},
		 	{"headersize", required_argument, 0, 'h'},
			{"imagesize", required_argument, 0, 'i'},
			{"file", required_argument, 0, 'f'},
			{"uuid", required_argument, 0, 'u'},
			{"clear-image", no_argument, 0, 'c'},
			{"header-only", no_argument, 0, 'H'},
			{0, 0, 0, 0}
		};

		opterr = 0; /* don't print errors */
		c = getopt_long(argc, argv, "dh:i:u:cHf:", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'd':
				dumpOnly = true;
				break;

			case 'h':
				headerSize = strtoull(optarg, NULL, 10);
				if (strchr(optarg, 'G') || strchr(optarg, 'g'))
					headerSize *= 1024 * 1024 * 1024;
				else if (strchr(optarg, 'M') || strchr(optarg, 'm'))
					headerSize *= 1024 * 1024;
				else if (strchr(optarg, 'K') || strchr(optarg, 'k'))
					headerSize *= 1024;
				break;

			case 'i':
				imageSize = strtoull(optarg, NULL, 10);
				if (strchr(optarg, 'G') || strchr(optarg, 'g'))
					imageSize *= 1024 * 1024 * 1024;
				else if (strchr(optarg, 'M') || strchr(optarg, 'm'))
					imageSize *= 1024 * 1024;
				else if (strchr(optarg, 'K') || strchr(optarg, 'k'))
					imageSize *= 1024;
				break;

			case 'u':
				uuid = optarg;
				if (!is_valid_uuid(uuid)) {
					fprintf(stderr, "Error: invalid UUID given (use "
						"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx format only).\n");
					exit(EXIT_FAILURE);
				}
				break;

			case 'f':
				file = optarg;
				break;

			case 'c':
				clearImage = true;
				break;

			case 'H':
				headerOnly = true;
				break;

			default:
				print_usage();
		}
	}

	if (file == NULL && optind == argc - 1)
		file = argv[optind];

	if (dumpOnly && file != NULL) {
		dump_image_info(file);
		return 0;
	}

	if (!headerSize || !imageSize || !file)
		print_usage();

	char desc[1024];
	SparseExtentHeader header;

	if (headerSize < sizeof(desc) + sizeof(header)) {
		fprintf(stderr, "Error: header size must be at least %u byte\n",
			(unsigned)(sizeof(desc) + sizeof(header)));
		exit(EXIT_FAILURE);
	}

	if (headerSize % 512) {
		fprintf(stderr, "Error: header size must be a multiple of 512 bytes\n");
		exit(EXIT_FAILURE);
	}

	if (imageSize % 512) {
		fprintf(stderr, "Error: image size must be a multiple of 512 bytes\n");
		exit(EXIT_FAILURE);
	}

	// arbitrary 1 GB limitation
	if (headerSize > 0x40000000ULL) {
		fprintf(stderr, "Error: header size too large\n");
		exit(EXIT_FAILURE);
	}

	// arbitrary 4 GB limitation
	if (imageSize > 0x100000000ULL) {
		fprintf(stderr, "Error: image size too large\n");
		exit(EXIT_FAILURE);
	}

	const char *name = strrchr(file, '/');
	name = name ? (name + 1) : file;

//	printf("headerSize %llu\n", headerSize);
//	printf("imageSize %llu\n", imageSize);
//	printf("file %s\n", file);

	uint64_t sectors;
	uint64_t heads;
	uint64_t cylinders;

	// TODO: fixme!
	sectors = 63;
	heads = 16;
	cylinders = imageSize / (sectors * heads * 512);
	while (cylinders > 1024) {
		cylinders /= 2;
		heads *= 2;
	}
	off_t actualImageSize = (off_t)cylinders * sectors * heads * 512;

	memset(desc, 0, sizeof(desc));
	memset(&header, 0, sizeof(header));

	header.magicNumber = VMDK_SPARSE_MAGICNUMBER;
	header.version = VMDK_SPARSE_VERSION;
	header.flags = 1;
	header.capacity = 0;
	header.grainSize = 16;
	header.descriptorOffset = 1;
	header.descriptorSize = (sizeof(desc) + 511) / 512;
	header.numGTEsPerGT = 512;
	header.rgdOffset = 0;
	header.gdOffset = 0;
	header.overHead = headerSize / 512;
	header.uncleanShutdown = 0;
	header.singleEndLineChar = '\n';
	header.nonEndLineChar = ' ';
	header.doubleEndLineChar1 = '\r';
	header.doubleEndLineChar2 = '\n';

	// Generate UUID for the image by hashing its full path
	uint64_t uuid1 = 0, uuid2 = 0, uuid3 = 0, uuid4 = 0, uuid5 = 0;
	if (uuid == NULL) {
		char fullPath[PATH_MAX + 6];
		strcpy(fullPath, "Haiku");

		if (realpath(file, fullPath + 5) == NULL)
			strncpy(fullPath + 5, file, sizeof(fullPath) - 5);

		size_t pathLength = strlen(fullPath);
		for (size_t i = pathLength; i < 42; i++) {
			// fill rest with some numbers
			fullPath[i] = i % 10 + '0';
		}
		if (pathLength < 42)
			fullPath[42] = '\0';

		uuid1 = hash_string(fullPath);
		uuid2 = hash_string(fullPath + 5);
		uuid3 = hash_string(fullPath + 13);
		uuid4 = hash_string(fullPath + 19);
		uuid5 = hash_string(fullPath + 29);
	}

	// Create embedded descriptor
	strcat(desc,
		"# Disk Descriptor File\n"
		"version=1\n"
		"CID=fffffffe\n"
		"parentCID=ffffffff\n"
		"createType=\"monolithicFlat\"\n");
	sprintf(desc + strlen(desc),
		"# Extent Description\n"
		"RW %llu FLAT \"%s\" %llu\n",
		(unsigned long long)actualImageSize / 512, name,
		(unsigned long long)headerSize / 512);
	sprintf(desc + strlen(desc),
		"# Disk Data Base\n"
		"ddb.toolsVersion = \"0\"\n"
		"ddb.virtualHWVersion = \"3\"\n"
		"ddb.geometry.sectors = \"%llu\"\n"
		"ddb.adapterType = \"ide\"\n"
		"ddb.geometry.heads = \"%llu\"\n"
		"ddb.geometry.cylinders = \"%llu\"\n",
		(unsigned long long)sectors, (unsigned long long)heads,
		(unsigned long long)cylinders);

	if (uuid == NULL) {
		sprintf(desc + strlen(desc),
			"ddb.uuid.image=\"%08llx-%04llx-%04llx-%04llx-%012llx\"\n",
			uuid1 & 0xffffffffLL, uuid2 & 0xffffLL, uuid3 & 0xffffLL,
			uuid4 & 0xffffLL, uuid5 & 0xffffffffffffLL);
	} else
		sprintf(desc + strlen(desc), "ddb.uuid.image=\"%s\"\n", uuid);

	int fd = open(file, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		fprintf(stderr, "Error: couldn't open file %s (%s)\n", file,
			strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (write(fd, &header, sizeof(header)) != sizeof(header))
		goto write_err;

	if (write(fd, desc, sizeof(desc)) != sizeof(desc))
		goto write_err;

	if ((uint64_t)lseek(fd, headerSize - 1, SEEK_SET) != headerSize - 1)
		goto write_err;

	if (1 != write(fd, "", 1))
		goto write_err;

	if (!headerOnly) {
		if ((clearImage && ftruncate(fd, headerSize) != 0)
			|| ftruncate(fd, actualImageSize + headerSize) != 0) {
			fprintf(stderr, "Error: resizing file %s failed (%s)\n", file,
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	close(fd);
	return 0;

write_err:
	fprintf(stderr, "Error: writing file %s failed (%s)\n", file,
		strerror(errno));
	close(fd);
	exit(EXIT_FAILURE);
}
