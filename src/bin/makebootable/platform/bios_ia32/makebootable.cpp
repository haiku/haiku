/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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

// Linux and FreeBSD support
#ifdef HAIKU_HOST_PLATFORM_LINUX
#	include <ctype.h>
#	include <linux/fs.h>
#	include <linux/hdreg.h>
#	include <sys/ioctl.h>

#	define USE_PARTITION_MAP 1
#elif HAIKU_HOST_PLATFORM_FREEBSD
#	include <ctype.h>
#	include <sys/disklabel.h>
#	include <sys/disk.h>
#	include <sys/ioctl.h>

#	define USE_PARTITION_MAP 1
#elif HAIKU_HOST_PLATFORM_DARWIN
#	include <ctype.h>
#	include <sys/disk.h>
#	include <sys/ioctl.h>

#	define USE_PARTITION_MAP 1
#endif

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <image.h>

#	include <DiskDevice.h>
#	include <DiskDeviceRoster.h>
#	include <Partition.h>
#	include <Path.h>

#	include "bfs_control.h"
#endif

#if USE_PARTITION_MAP
#	include "guid.h"
#	include "gpt_known_guids.h"
#	include "Header.h"
#	include "PartitionMap.h"
#	include "PartitionMapParser.h"
#endif


static const char *kCommandName = "makebootable";

static const int kBootCodeSize				= 1024;
static const int kFirstBootCodePartSize		= 512;
static const int kSecondBootCodePartOffset	= 676;
static const int kSecondBootCodePartSize	= kBootCodeSize
												- kSecondBootCodePartOffset;
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


// read_boot_code_data
static uint8 *
read_boot_code_data(const char* programPath)
{
	// open our executable
	BFile executableFile;
	status_t error = executableFile.SetTo(programPath, B_READ_ONLY);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to open my executable file (\"%s\": "
			"%s\n", programPath, strerror(error));
		exit(1);
	}

	uint8 *bootCodeData = new uint8[kBootCodeSize];

	// open our resources
	BResources resources;
	error = resources.SetTo(&executableFile);
	const void *resourceData = NULL;
	if (error == B_OK) {
		// read the boot block from the resources
		size_t resourceSize;
		resourceData = resources.LoadResource(B_RAW_TYPE, 666, &resourceSize);

		if (resourceData && resourceSize != (size_t)kBootCodeSize) {
			resourceData = NULL;
			printf("Warning: Something is fishy with my resources! The boot "
				"code doesn't have the correct size. Trying the attribute "
				"instead ...\n");
		}
	}

	if (resourceData) {
		// found boot data in the resources
		memcpy(bootCodeData, resourceData, kBootCodeSize);
	} else {
		// no boot data in the resources; try the attribute
		ssize_t bytesRead = executableFile.ReadAttr("BootCode", B_RAW_TYPE,
			0, bootCodeData, kBootCodeSize);
		if (bytesRead < 0) {
			fprintf(stderr, "Error: Failed to read boot code from resources "
				"or attribute.\n");
			exit(1);
		}
		if (bytesRead != kBootCodeSize) {
			fprintf(stderr, "Error: Failed to read boot code from resources, "
				"and the boot code in the attribute has the wrong size!\n");
			exit(1);
		}
	}

	return bootCodeData;
}


// write_boot_code_part
static void
write_boot_code_part(const char *fileName, int fd, off_t imageOffset,
	const uint8 *bootCodeData, int offset, int size, bool dryRun)
{
	if (!dryRun) {
		ssize_t bytesWritten = write_pos(fd, imageOffset + offset,
			bootCodeData + offset, size);
		if (bytesWritten != size) {
			fprintf(stderr, "Error: Failed to write to \"%s\": %s\n", fileName,
				strerror(bytesWritten < 0 ? errno : B_ERROR));
		}
	}
}


#ifdef HAIKU_TARGET_PLATFORM_HAIKU

static status_t
find_own_image(image_info *info)
{
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, info) == B_OK) {
		if (((addr_t)info->text <= (addr_t)find_own_image
			&& (addr_t)info->text + info->text_size
				> (addr_t)find_own_image)) {
			return B_OK;
		}
	}

	return B_NAME_NOT_FOUND;
}

#endif


#if USE_PARTITION_MAP

static void
dump_partition_map(const PartitionMap& map)
{
	fprintf(stderr, "partitions:\n");
	int32 count = map.CountPartitions();
	for (int i = 0; i < count; i++) {
		const Partition* partition = map.PartitionAt(i);
		fprintf(stderr, "%2d: ", i);
		if (partition == NULL) {
			fprintf(stderr, "<null>\n");
			continue;
		}

		if (partition->IsEmpty()) {
			fprintf(stderr, "<empty>\n");
			continue;
		}

		fprintf(stderr, "offset: %16" B_PRIdOFF ", size: %16" B_PRIdOFF
			", type: %x%s\n", partition->Offset(), partition->Size(),
			partition->Type(), partition->IsExtended() ? " (extended)" : "");
	}
}


static void
get_partition_offset(int deviceFD, off_t deviceStart, off_t deviceSize,
		int blockSize, int partitionIndex, char *deviceName,
		int64 &_partitionOffset)
{
	PartitionMapParser parser(deviceFD, deviceStart, deviceSize, blockSize);
	PartitionMap map;
	status_t error = parser.Parse(NULL, &map);
	if (error == B_OK) {
		Partition *partition = map.PartitionAt(partitionIndex - 1);
		if (!partition || partition->IsEmpty()) {
			fprintf(stderr, "Error: Invalid partition index %d.\n",
				partitionIndex);
			dump_partition_map(map);
			exit(1);
		}

		if (partition->IsExtended()) {
			fprintf(stderr, "Error: Partition %d is an extended "
				"partition.\n", partitionIndex);
			dump_partition_map(map);
			exit(1);
		}

		_partitionOffset = partition->Offset();
	} else {
		// try again using GPT instead
		EFI::Header gptHeader(deviceFD, deviceSize, blockSize);
		error = gptHeader.InitCheck();
		if (error == B_OK && partitionIndex < gptHeader.EntryCount()) {
			efi_partition_entry partition = gptHeader.EntryAt(partitionIndex - 1);

			static_guid bfs_uuid = {0x42465331, 0x3BA3, 0x10F1,
				0x802A4861696B7521LL};

			if (!(bfs_uuid == partition.partition_type)) {
				fprintf(stderr, "Error: Partition %d does not have the "
					"BFS UUID.\n", partitionIndex);
				exit(1);
			}

			_partitionOffset = partition.StartBlock() * blockSize;
		} else {
			fprintf(stderr, "Error: Parsing partition table on device "
				"\"%s\" failed: %s\n", deviceName, strerror(error));
			exit(1);
		}
	}

	close(deviceFD);
}

#endif


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
	off_t startOffset = 0;

	// parse arguments
	for (int argi = 1; argi < argc;) {
		const char *arg = argv[argi++];

		if (arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);
			} else if (strcmp(arg, "--dry-run") == 0) {
				dryRun = true;
			} else if (strcmp(arg, "--start-offset") == 0) {
				if (argi >= argc)
					print_usage_and_exit(true);
				startOffset = strtoll(argv[argi++], NULL, 0);
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

	// read the boot code
	uint8 *bootCodeData = NULL;
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	bootCodeData = read_boot_code_data(argv[0]);
#else
	image_info info;
	if (find_own_image(&info) == B_OK)
		bootCodeData = read_boot_code_data(info.name);
#endif
	if (!bootCodeData) {
		fprintf(stderr, "Error: Failed to read \n");
		exit(1);
	}

	// iterate through the files and make them bootable
	status_t error;
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
			#ifdef HAIKU_TARGET_PLATFORM_HAIKU

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
			// or under FreeBSD
			#if !defined(HAIKU_TARGET_PLATFORM_HAIKU) \
				&& !defined(HAIKU_HOST_PLATFORM_FREEBSD)

				fprintf(stderr, "Error: Character special devices not "
					"supported on this platform.\n");
				exit(1);

			#endif

			#ifdef HAIKU_HOST_PLATFORM_FREEBSD

				// chop off the trailing number
				int fileNameLen = strlen(fileName);
				int baseNameLen = -1;
				for (int k = fileNameLen - 1; k >= 0; k--) {
					if (!isdigit(fileName[k])) {
						baseNameLen = k + 1;
						break;
					}
				}

				// Remove de 's' from 'ad2s2' slice device (partition for DOS
				// users) to get 'ad2' base device
				baseNameLen--;

				if (baseNameLen < 0) {
					// only digits?
					fprintf(stderr, "Error: Failed to get base device name.\n");
					exit(1);
				}

				if (baseNameLen < fileNameLen) {
					// get base device name and partition index
					char baseDeviceName[B_PATH_NAME_LENGTH];
					int partitionIndex = atoi(fileName + baseNameLen + 1);
						// Don't forget the 's' of slice :)
					memcpy(baseDeviceName, fileName, baseNameLen);
					baseDeviceName[baseNameLen] = '\0';

					// open base device
					int baseFD = open(baseDeviceName, O_RDONLY);
					if (baseFD < 0) {
						fprintf(stderr, "Error: Failed to open \"%s\": %s\n",
							baseDeviceName, strerror(errno));
						exit(1);
					}

					// get device size
					int64 deviceSize;
					if (ioctl(baseFD, DIOCGMEDIASIZE, &deviceSize) == -1) {
						fprintf(stderr, "Error: Failed to get device geometry "
							"for \"%s\": %s\n", baseDeviceName,
							strerror(errno));
						exit(1);
					}

					// parse the partition map
					// TODO: block size!
					get_partition_offset(baseFD, 0, deviceSize, 512,
						partitionIndex, baseDeviceName, partitionOffset);
				} else {
					// The given device is the base device. We'll write at
					// offset 0.
				}

			#endif // HAIKU_HOST_PLATFORM_FREEBSD

		} else if (S_ISBLK(st.st_mode)) {
			// block device: a device or partition under Linux or Darwin
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

					// get device size -- try BLKGETSIZE64, but, if it doesn't
					// work, fall back to the obsolete HDIO_GETGEO
					int64 deviceSize;
					hd_geometry geometry;
					if (ioctl(baseFD, BLKGETSIZE64, &deviceSize) == 0
						&& deviceSize > 0) {
						// looks good
					} else if (ioctl(baseFD, HDIO_GETGEO, &geometry) == 0) {
						deviceSize = (int64)geometry.heads * geometry.sectors
							* geometry.cylinders * 512;
					} else {
						fprintf(stderr, "Error: Failed to get device geometry "
							"for \"%s\": %s\n", baseDeviceName,
							strerror(errno));
						exit(1);
					}

					// parse the partition map
					// TODO: block size!
					get_partition_offset(baseFD, 0, deviceSize, 512,
						partitionIndex, baseDeviceName, partitionOffset);
				} else {
					// The given device is the base device. We'll write at
					// offset 0.
				}

			#elif defined(HAIKU_HOST_PLATFORM_DARWIN)
				// chop off the trailing number
				int fileNameLen = strlen(fileName);
				int baseNameLen = fileNameLen - 2;

				// get base device name and partition index
				char baseDeviceName[B_PATH_NAME_LENGTH];
				int partitionIndex = atoi(fileName + baseNameLen + 1);
				memcpy(baseDeviceName, fileName, baseNameLen);
				baseDeviceName[baseNameLen] = '\0';

				// open base device
				int baseFD = open(baseDeviceName, O_RDONLY);
				if (baseFD < 0) {
					fprintf(stderr, "Error: Failed to open \"%s\": %s\n",
							baseDeviceName, strerror(errno));
					exit(1);
				}

				// get device size
				int64 blockSize;
				int64 blockCount;
				int64 deviceSize;
				if (ioctl(baseFD, DKIOCGETBLOCKSIZE, &blockSize) == -1) {
					fprintf(stderr, "Error: Failed to get block size "
							"for \"%s\": %s\n", baseDeviceName,
							strerror(errno));
					exit(1);
				}
				if (ioctl(baseFD, DKIOCGETBLOCKCOUNT, &blockCount) == -1) {
					fprintf(stderr, "Error: Failed to get block count "
							"for \"%s\": %s\n", baseDeviceName,
							strerror(errno));
					exit(1);
				}

				deviceSize = blockSize * blockCount;

				// parse the partition map
				get_partition_offset(baseFD, 0, deviceSize, 512,
						partitionIndex, baseDeviceName, partitionOffset);
			#else
			// partitions are block devices under Haiku, but not under BeOS
			#ifdef HAIKU_TARGET_PLATFORM_HAIKU
				fprintf(stderr, "Error: Block devices not supported on this "
					"platform!\n");
				exit(1);
			#endif	// HAIKU_TARGET_PLATFORM_HAIKU

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

		#ifdef HAIKU_TARGET_PLATFORM_HAIKU

			// get a partition info
			if (!noPartition
				&& strlen(fileName) >= 3
				&& strncmp("raw", fileName + strlen(fileName) - 3, 3)) {
				partition_info partitionInfo;
				if (ioctl(fd, B_GET_PARTITION_INFO, &partitionInfo,
						sizeof(partitionInfo)) == 0) {
					partitionOffset = partitionInfo.offset;
				} else {
					fprintf(stderr, "Error: Failed to get partition info: %s\n",
						strerror(errno));
					exit(1);
				}
			}

		#endif	// HAIKU_TARGET_PLATFORM_HAIKU

		// adjust the partition offset in the boot code data
		// hard coded sector size: 512 bytes
		*(uint32*)(bootCodeData + kPartitionOffsetOffset)
			= B_HOST_TO_LENDIAN_INT32((uint32)(partitionOffset / 512));

		// write the boot code
		printf("Writing boot code to \"%s\" (partition offset: %" B_PRId64
			" bytes, start offset = %" B_PRIdOFF ") "
			"...\n", fileName, partitionOffset, startOffset);

		write_boot_code_part(fileName, fd, startOffset, bootCodeData, 0,
			kFirstBootCodePartSize, dryRun);
		write_boot_code_part(fileName, fd, startOffset, bootCodeData,
			kSecondBootCodePartOffset, kSecondBootCodePartSize,
			dryRun);

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		// check if this partition is mounted
		BDiskDeviceRoster roster;
		BPartition* partition;
		BDiskDevice device;
		status_t status = roster.GetPartitionForPath(fileName, &device,
			&partition);
		if (status != B_OK) {
			status = roster.GetFileDeviceForPath(fileName, &device);
			if (status == B_OK)
				partition = &device;
		}
		if (status == B_OK && partition->IsMounted() && !dryRun) {
			// This partition is mounted, we need to tell BFS to update its
			// boot block (we are using part of the same logical block).
			BPath path;
			status = partition->GetMountPoint(&path);
			if (status == B_OK) {
				update_boot_block update;
				update.offset = kSecondBootCodePartOffset - 512;
				update.data = bootCodeData + kSecondBootCodePartOffset;
				update.length = kSecondBootCodePartSize;

				int mountFD = open(path.Path(), O_RDONLY);
				if (ioctl(mountFD, BFS_IOCTL_UPDATE_BOOT_BLOCK, &update,
						sizeof(update_boot_block)) != 0) {
					fprintf(stderr, "Could not update BFS boot block: %s\n",
						strerror(errno));
				}
				close(mountFD);
			} else {
				fprintf(stderr, "Could not update BFS boot code while the "
					"partition is mounted!\n");
			}
		}
#endif	// HAIKU_TARGET_PLATFORM_HAIKU

		close(fd);
	}

	return 0;
}
