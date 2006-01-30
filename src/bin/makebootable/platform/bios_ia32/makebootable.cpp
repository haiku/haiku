/*
 * Copyright 2005-2006, Ingo Weinhold, bonefish@users.sf.net.
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

// Linux support
#ifdef HAIKU_HOST_PLATFORM_LINUX
#	include <ctype.h>
#	include <linux/hdreg.h>
#	include <sys/ioctl.h>

#	include "PartitionMap.h"
#	include "PartitionMapParser.h"
#endif	// HAIKU_HOST_PLATFORM_LINUX


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
"  --dry-run     - Do everything but actually writing the boot block to disk.\n"
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
	int offset, int size, bool dryRun)
{
	printf("writing %d bytes at offset %d%s\n", size, offset,
		(dryRun ? " (dry run)" : ""));

	if (!dryRun) {
		ssize_t bytesWritten = write_pos(fd, offset, bootCodeData + offset,
			size);
		if (bytesWritten != size) {
			fprintf(stderr, "Error: Failed to write to \"%s\": %s\n", fileName,
				strerror(bytesWritten < 0 ? errno : B_ERROR));
		}
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
	bool dryRun = false;

	// parse arguments
	for (int argi = 1; argi < argc;) {
		const char *arg = argv[argi++];

		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "--dry-run") == 0) {
				dryRun = true;
			} else if (strcmp(arg, "-alert") == 0) {
				// ignore
			} else if (strcmp(arg, "-full") == 0) {
				// ignore
			} else if (strcmp(arg, "-safe") == 0) {
				fprintf(stderr, "Error: Sorry, BeOS R3 isn't supported!\n");
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
			"doesn't have the correct size.\n");
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
		int64 partitionOffset = 0;
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
				fprintf(stderr, "Error: Specifying directories not supported "
					"on this platform!\n");
				exit(1);
			
			#endif

		} else if (S_ISREG(st.st_mode)) {
			// a regular file: fine
			noPartition = true;
		} else if (S_ISCHR(st.st_mode)) {
			// character special: a device or partition under BeOS
			#ifndef __BEOS__

				fprintf(stderr, "Error: Character special devices not "
					"supported on this platform.\n");
				exit(1);

			#endif
		} else if (S_ISBLK(st.st_mode)) {
			// block device: a device or partition under Linux
			#ifdef HAIKU_HOST_PLATFORM_LINUX

				// chop off the trailing number
				int fileNameLen = strlen(fileName);
				int baseNameLen = -1;
				for (int k = fileNameLen - 1; k >= 0; k--) {
					if (!isdigit(fileName[k])) {
						baseNameLen = k + 1;
						break;
					}
				}

				if (baseNameLen < 0) {
					// only digits?
					fprintf(stderr, "Error: Failed to get base device name.\n");
					exit(1);
				}

				if (baseNameLen < fileNameLen) {
					// get base device name and partition index
					char baseDeviceName[B_PATH_NAME_LENGTH];
					int partitionIndex = atoi(fileName + baseNameLen);
					memcpy(baseDeviceName, fileName, baseNameLen);
					baseDeviceName[baseNameLen] = '\0';

					// open base device
					int baseFD = open(baseDeviceName, O_RDONLY);
					if (baseFD < 0) {
						fprintf(stderr, "Error: Failed to open \"%s\": %s\n",
							baseDeviceName, strerror(errno));
						exit(1);
					}

					// get device geometry
					hd_geometry geometry;
					if (ioctl(baseFD, HDIO_GETGEO, &geometry) < 0) {
						fprintf(stderr, "Error: Failed to get device geometry "
							"for \"%s\": %s\n", baseDeviceName,
							strerror(errno));
						exit(1);
					}
					int64 deviceSize = (int64)geometry.heads * geometry.sectors
						* geometry.cylinders * 512;

					// parse the partition map
					PartitionMapParser parser(baseFD, 0, deviceSize, 512);
					PartitionMap map;
					error = parser.Parse(NULL, &map);
					if (error != B_OK) {
						fprintf(stderr, "Error: Parsing partition table on "
							"device \"%s\" failed: %s\n", baseDeviceName,
							strerror(error));
						exit(1);
					}

					close(baseFD);

					// check the partition we are supposed to write at
					Partition *partition = map.PartitionAt(partitionIndex - 1);
					if (!partition || partition->IsEmpty()) {
						fprintf(stderr, "Error: Invalid partition index %d.\n",
							partitionIndex);
						exit(1);
					}

					if (partition->IsExtended()) {
						fprintf(stderr, "Error: Partition %d is an extended "
							"partition.\n", partitionIndex);
						exit(1);
					}

					partitionOffset = partition->Offset();

				} else {
					// The given device is the base device. We'll write at
					// offset 0.
				}
			
			#else	// !HAIKU_HOST_PLATFORM_LINUX

				fprintf(stderr, "Error: Block devices not supported on this "
					"platform!\n");
				exit(1);

			#endif
		} else {
			fprintf(stderr, "Error: File type of \"%s\" is not supported.\n",
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

		#ifdef __BEOS__

			// get a partition info
			if (!noPartition) {
				partition_info partitionInfo;
				if (ioctl(fd, B_GET_PARTITION_INFO, &partitionInfo) == 0) {
					// hard coded sector size: 512 bytes
					partitionOffset = partitionInfo.offset / 512;
				} else {
					fprintf(stderr, "Error: Failed to get partition info: %s",
						strerror(errno));
					exit(1);
				}
			}

		#endif	// __BEOS__

		// adjust the partition offset in the boot code data
		// hard coded sector size: 512 bytes
		*(uint32*)(bootCodeData + kPartitionOffsetOffset)
			= B_HOST_TO_LENDIAN_INT32((uint32)(partitionOffset / 512));

		// write the boot code
		printf("Writing boot code to \"%s\" (partition offset: %lld bytes) "
			"...\n", fileName, partitionOffset);

		write_boot_code_part(fileName, fd, bootCodeData, 0,
			kFirstBootCodePartSize, dryRun);
		write_boot_code_part(fileName, fd, bootCodeData,
			kSecondBootcodePartOffset, kSecondBootcodePartSize, dryRun);

		close(fd);
	}

	return 0;
}
