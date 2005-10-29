/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <ByteOrder.h>
#include <Drivers.h>
#include <Entry.h>
#include <File.h>
#include <fs_info.h>
#include <Resources.h>
#include <TypeConstants.h>

static const char *kCommandName = "makebootable";

static const int kBootCodeSize				= 1024;
static const int kFirstBootCodePartSize		= 512;
static const int kSecondBootcodePartOffset	= 676;
static const int kSecondBootcodePartSize	= kBootCodeSize
												- kSecondBootcodePartOffset;
static const int kPartitionOffsetOffset		= 506;

static int kArgc;
static const char *const *kArgv;

// usage
const char *kUsage =
"Usage: %s [ options ] <file> ...\n"
"\n"
"Makes the specified BFS partitions/devices bootable by writing boot code\n"
"into the first two sectors. It doesn't mark the partition(s) active.\n"
"\n"
"If a given <file> refers to a directory, the partition/device on which the\n"
"directory resides will be made bootable. If it refers to a regular file,\n"
"the file is considered a disk image and the boot code will be written to\n"
"it.\n"
"\n"
"Options:\n"
"  -h, --help    - Print this help text and exit.\n"
"\n"
"[compatibility]\n"
"  -alert        - Compatibility option. Ignored.\n"
"  -full         - Compatibility option. Ignored.\n"
"  -safe         - Compatibility option. Fail when specified.\n"
;

// print_usage
static void
print_usage(bool error)
{
	// get command name
	const char *commandName = NULL;
	if (kArgc > 0) {
		if (const char *lastSlash = strchr(kArgv[0], '/'))
			commandName = lastSlash + 1;
		else
			commandName = kArgv[0];
	}

	if (!commandName || strlen(commandName) == 0)
		commandName = kCommandName;

	// print usage
	fprintf((error ? stderr : stdout), kUsage, commandName, commandName,
		commandName);
}

// print_usage_and_exit
static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}

// write_boot_code_part
static void
write_boot_code_part(const char *fileName, int fd, const uint8 *bootCodeData,
	int offset, int size)
{
printf("writing %d bytes at offset %d\n", size, offset);
	ssize_t bytesWritten = write_pos(fd, offset, bootCodeData + offset, size);
	if (bytesWritten != size) {
		fprintf(stderr, "Error: Failed to write to \"%s\": %s\n", fileName,
			strerror(bytesWritten < 0 ? errno : B_ERROR));
	}
}

// main
int
main(int argc, const char *const *argv)
{
	kArgc = argc;
	kArgv = argv;

	if (argc < 2)
		print_usage_and_exit(true);

	// parameters
	const char **files = new const char*[argc];
	int fileCount = 0;

	// parse arguments
	for (int argi = 1; argi < argc;) {
		const char *arg = argv[argi++];

		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "-alert") == 0) {
				// ignore
			} else if (strcmp(arg, "-full") == 0) {
				// ignore
			} else if (strcmp(arg, "-safe") == 0) {
				fprintf(stderr, "Error: Sorry, BeOS R3 isnt't supported!\n");
				exit(1);
			} else {
				print_usage_and_exit(true);
			}

		} else {
			files[fileCount++] = arg;
		}
	}

	// we need at least one file
	if (fileCount == 0)
		print_usage_and_exit(true);

	// open our executable
	BFile resourcesFile;
	status_t error = resourcesFile.SetTo(argv[0], B_READ_ONLY);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to open my resources: %s\n",
			strerror(error));
		exit(1);
	}

	// open our resources
	BResources resources;
	error = resources.SetTo(&resourcesFile);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to read my resources: %s\n",
			strerror(error));
		exit(1);
	}

	// read the boot block from the resources
	size_t resourceSize;
	const void *resourceData = resources.LoadResource(B_RAW_TYPE, 666,
		&resourceSize);
	if (!resourceData) {
		fprintf(stderr,
			"Error: Failed to read the boot block from my resources!\n");
		exit(1);
	}

	if (resourceSize != (size_t)kBootCodeSize) {
		fprintf(stderr,
			"Error: Something is fishy with my resources! The boot code "
			"doesn't have the correct size\n");
		exit(1);
	}

	// clone the boot code data, so that we can modify it
	uint8 *bootCodeData = new uint8[kBootCodeSize];
	memcpy(bootCodeData, resourceData, kBootCodeSize);

	// iterate through the files and make them bootable
	for (int i = 0; i < fileCount; i++) {
		const char *fileName = files[i];
		BEntry entry;
		error = entry.SetTo(fileName, true);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open \"%s\": %s\n",
				fileName, strerror(error));
			exit(1);
		}

		// get stat to check the type of the file
		struct stat st;
		error = entry.GetStat(&st);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to stat \"%s\": %s\n",
				fileName, strerror(error));
			exit(1);
		}

		bool noPartition = false;
		fs_info info;	// needs to be here (we use the device name later)
		if (S_ISDIR(st.st_mode)) {
			#ifdef __BEOS__

			// a directory: get the device
			error = fs_stat_dev(st.st_dev, &info);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to determine device for "
					"\"%s\": %s\n", fileName, strerror(error));
				exit(1);
			}

			fileName = info.device_name;

			#else

			(void)info;
			fprintf(stderr, "Error: Only image files are supported on this "
				"platform!\n");
			exit(1);
			
			#endif

		} else if (S_ISREG(st.st_mode)) {
			// a regular file: fine
			noPartition = true;
		} else if (S_ISCHR(st.st_mode)) {
			// a device or partition
			#ifndef __BEOS__

			fprintf(stderr, "Error: Only image files are supported on this "
				"platform!\n");
			exit(1);

			#endif
		} else {
			fprintf(stderr, "Error: File type of \"%s\" is not unsupported\n",
				fileName);
			exit(1);
		}

		// open the file
		int fd = open(fileName, O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "Error: Failed to open \"%s\": %s\n", fileName,
				strerror(errno));
			exit(1);
		}

		// get a partition info
		int64 partitionOffset = 0;
		
		#ifdef __BEOS__

		partition_info partitionInfo;
		if (ioctl(fd, B_GET_PARTITION_INFO, &partitionInfo) == 0) {
			// hard coded sector size: 512 bytes
			partitionOffset = partitionInfo.offset / 512;
		}

		#endif	// __BEOS__

		//  adjust the partition offset in the boot code data
		*(uint32*)(bootCodeData + kPartitionOffsetOffset)
			= B_HOST_TO_LENDIAN_INT32((uint32)partitionOffset);

		// write the boot code
		printf("Writing boot code to \"%s\" ...\n", fileName);

		write_boot_code_part(fileName, fd, bootCodeData, 0,
			kFirstBootCodePartSize);
		write_boot_code_part(fileName, fd, bootCodeData,
			kSecondBootcodePartOffset, kSecondBootcodePartSize);

		close(fd);
	}

	return 0;
}
