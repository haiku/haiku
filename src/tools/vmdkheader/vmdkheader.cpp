/*
 * Copyright 2007, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vmdkheader.h"


static void
print_usage()
{
	printf("\n");
	printf("vmdkheader\n");
	printf("\n");
	printf("usage: vmdkheader -i <imagesize> -h <headersize> [-f] <file>\n");
	printf("       -i, --imagesize    size of raw partition image file\n");
	printf("       -h, --headersize   size of the vmdk header to write\n");
	printf("       -f, --file         the raw partition image file\n");
	exit(EXIT_FAILURE);
}


int
main(int argc, char *argv[])
{
	uint64 headersize = 0;
	uint64 imagesize = 0;
	const char *file = NULL;

	if (sizeof(SparseExtentHeader) != 512) {
		fprintf(stderr, "compilation error: struct size is %u byte\n", sizeof(SparseExtentHeader));
		exit(EXIT_FAILURE);
	}

	while (1) { 
		int c;
		static struct option long_options[] = 
		{ 
		 	{"headersize", required_argument, 0, 'h'}, 
			{"imagesize", required_argument, 0, 'i'}, 
			{"file", required_argument, 0, 'f'}, 
			{0, 0, 0, 0} 
		};

		opterr = 0; /* don't print errors */
		c = getopt_long(argc, argv, "h:i:f:", long_options, NULL); 
		if (c == -1) 
			break; 

		switch (c) { 
			case 'h':
				headersize = strtoull(optarg, NULL, 10);
				if (strchr(optarg, 'G') || strchr(optarg, 'g'))
					headersize *= 1024 * 1024 * 1024;
				else if (strchr(optarg, 'M') || strchr(optarg, 'm'))
					headersize *= 1024 * 1024;
				else if (strchr(optarg, 'K') || strchr(optarg, 'k'))
					headersize *= 1024;
				break; 

			case 'i':
				imagesize = strtoull(optarg, NULL, 10);
				if (strchr(optarg, 'G') || strchr(optarg, 'g'))
					imagesize *= 1024 * 1024 * 1024;
				else if (strchr(optarg, 'M') || strchr(optarg, 'm'))
					imagesize *= 1024 * 1024;
				else if (strchr(optarg, 'K') || strchr(optarg, 'k'))
					imagesize *= 1024;
				break; 

			case 'f':
				file = optarg;
				break; 

			default: 
				print_usage();
		} 
	} 

	if (file == NULL && optind == argc - 1)
		file = argv[optind];

	if (!headersize || !imagesize || !file)
		print_usage();

	char desc[512];
	SparseExtentHeader header;

	if (headersize < sizeof(desc) + sizeof(header)) {
		fprintf(stderr, "Error: header size must be at least %u byte\n", sizeof(desc) + sizeof(header));
		exit(EXIT_FAILURE);
	}

	if (headersize % 512) {
		fprintf(stderr, "Error: header size must be a multiple of 512 bytes\n");
		exit(EXIT_FAILURE);
	}

	if (imagesize % 512) {
		fprintf(stderr, "Error: image size must be a multiple of 512 bytes\n");
		exit(EXIT_FAILURE);
	}

	// arbitrary 1 GB limitation
	if (headersize > 0x40000000u) {
		fprintf(stderr, "Error: header size too large\n");
		exit(EXIT_FAILURE);
	}

	// arbitrary 2 GB limitation
	if (imagesize > 0x80000000u) {
		fprintf(stderr, "Error: image size too large\n");
		exit(EXIT_FAILURE);
	}

	const char *name = strrchr(file, '/');
	name = name ? (name + 1) : file;

//	printf("headersize %llu\n", headersize);
//	printf("imagesize %llu\n", imagesize);
//	printf("file %s\n", file);

	uint64 sectors;
	uint64 heads;
	uint64 cylinders;

	// TODO: fixme!
	sectors = 63;
	heads = 16;
	cylinders = imagesize / (sectors * heads * 512);
	while (cylinders > 1024) {
		cylinders /= 2;
		heads *= 2;
	}

	memset(desc, 0, sizeof(desc));
	memset(&header, 0, sizeof(header));

	header.magicNumber = SPARSE_MAGICNUMBER;
	header.version = 1;
	header.flags = 1;
	header.capacity = 0;
	header.grainSize = 16;
	header.descriptorOffset = 1;
	header.descriptorSize = sizeof(desc) / 512;
	header.numGTEsPerGT = 512;
	header.rgdOffset = 0;
	header.gdOffset = 0;
	header.overHead = headersize / 512;
	header.uncleanShutdown = 0;
	header.singleEndLineChar = '\n';
	header.nonEndLineChar = ' ';
	header.doubleEndLineChar1 = '\r';
	header.doubleEndLineChar2 = '\n';

	strcat(desc, 
		"# Disk Descriptor File\n"
		"version=1\n"
		"CID=fffffffe\n"
		"parentCID=ffffffff\n"
		"createType=\"monolithicFlat\"\n");
	sprintf(desc + strlen(desc),
		"# Extent Description\n"
		"RW %llu FLAT \"%s\" %llu\n",
		imagesize / 512, name, headersize / 512);
	sprintf(desc + strlen(desc), 
		"# Disk Data Base\n"
		"ddb.toolsVersion = \"0\"\n"
		"ddb.virtualHWVersion = \"3\"\n"
		"ddb.geometry.sectors = \"%llu\"\n"
		"ddb.adapterType = \"ide\"\n"
		"ddb.geometry.heads = \"%llu\"\n"
		"ddb.geometry.cylinders = \"%llu\"\n",
		sectors, heads, cylinders);
	
	int fd = open(file, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		fprintf(stderr, "Error: couldn't open file %s (%s)\n", file, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (sizeof(header) != write(fd, &header, sizeof(header)))
		goto write_err;

	if (sizeof(desc) != write(fd, desc, sizeof(desc)))
		goto write_err;
	
	headersize--;
	if (headersize != (uint64)lseek(fd, headersize, SEEK_SET))
		goto write_err;

	if (1 != write(fd, "", 1))
		goto write_err;

	close(fd);
	return 0;

write_err:
	fprintf(stderr, "Error: writing file %s failed (%s)\n", file, strerror(errno));
	close(fd);
	exit(EXIT_FAILURE);
}
