/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <Path.h>
#include <String.h>

#include <AutoDeleter.h>
#include <StringForSize.h>
#include <TextTable.h>

#include <file_systems/ram_disk/ram_disk.h>


extern const char* __progname;
static const char* kProgramName = __progname;

static const char* const kUsage =
	"Usage: %s <command> [ <options> ]\n"
	"Controls RAM disk devices.\n"
	"\n"
	"Commands:\n"
	"  create (-s <size> | <path>)\n"
	"    Creates a new RAM disk.\n"
	"  delete <id>\n"
	"    Deletes an existing RAM disk.\n"
	"  flush <id>\n"
	"    Writes modified data of an existing RAM disk back to its file.\n"
	"  help\n"
	"    Print this usage info.\n"
	"  list\n"
	"    List all RAM disks.\n"
;

static const char* const kCreateUsage =
	"Usage: %s %s (-s <size> | <path>)\n"
	"Creates a new RAM disk device. If the <size> argument is specified, a\n"
	"new zeroed RAM disk with that size (in bytes, suffixes 'k', 'm', 'g' are\n"
	"interpreted as KiB, MiB, GiB) is registered.\n"
	"Alternatively a file path can be specified. In that case the RAM disk \n"
	"data are initially read from that file and at any later point the\n"
	"modified RAM disk data can be written back to the same file upon request\n"
	"(via the \"flush\" command). The size of the RAM disk is implied by that\n"
	"of the file.\n"
;

static const char* const kDeleteUsage =
	"Usage: %s %s <id>\n"
	"Deletes the existing RAM disk with ID <id>. All modified data will be\n"
	"lost.\n"
;

static const char* const kFlushUsage =
	"Usage: %s %s <id>\n"
	"Writes all modified data of the RAM disk with ID <id> back to the file\n"
	"specified when the RAM disk was created. Fails, if the RAM disk had been\n"
	"created without an associated file.\n"
;

static const char* const kListUsage =
	"Usage: %s %s\n"
	"Lists all existing RAM disks.\n"
;

static const char* const kRamDiskControlDevicePath
	= "/dev/" RAM_DISK_CONTROL_DEVICE_NAME;
static const char* const kRamDiskRawDeviceBasePath
	= "/dev/" RAM_DISK_RAW_DEVICE_BASE_NAME;

static const char* sCommandName = NULL;
static const char* sCommandUsage = NULL;


static void
print_usage_and_exit(bool error)
{
	if (sCommandUsage != NULL) {
	    fprintf(error ? stderr : stdout, sCommandUsage, kProgramName,
			sCommandName);
	} else
	    fprintf(error ? stderr : stdout, kUsage, kProgramName);
    exit(error ? 1 : 0);
}


static status_t
execute_control_device_ioctl(int operation, void* request)
{
	// open the ram disk control device
	int fd = open(kRamDiskControlDevicePath, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error: Failed to open RAM disk control device \"%s\": "
			"%s\n", kRamDiskControlDevicePath, strerror(errno));
		return errno;
	}
	FileDescriptorCloser fdCloser(fd);

	// issue the request
	if (ioctl(fd, operation, request) < 0)
		return errno;

	return B_OK;
}


static int
command_register(int argc, const char* const* argv)
{
	sCommandUsage = kCreateUsage;

	int64 deviceSize = -1;

	while (true) {
		static struct option sLongOptions[] = {
			{ "size", required_argument, 0, 's' },
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+s:h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			case 's':
			{
				const char* sizeString = optarg;
				deviceSize = parse_size(sizeString);

				if (deviceSize <= 0) {
					fprintf(stderr, "Error: Invalid size argument: \"%s\"\n",
						sizeString);
					return 1;
				}

				// check maximum size
				system_info info;
				get_system_info(&info);
				if (deviceSize / B_PAGE_SIZE > (int64)info.max_pages * 2 / 3) {
					fprintf(stderr, "Error: Given RAM disk size too large.\n");
					return 1;
				}

				break;
			}

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining optional argument is the file path. It may only be
	// specified, if no size has been specified.
	const char* path = optind < argc ? argv[optind++] : NULL;
	if (optind < argc || (deviceSize >= 0) == (path != NULL))
		print_usage_and_exit(true);

	// prepare the request
	ram_disk_ioctl_register request;
	request.size = (uint64)deviceSize;
	request.path[0] = '\0';
	request.id = -1;

	if (path != NULL) {
		// verify the path
		BEntry entry;
		status_t error = entry.SetTo(path, true);
		if (error == B_OK && !entry.Exists())
			error = B_ENTRY_NOT_FOUND;
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to resolve path \"%s\": %s\n",
				path, strerror(error));
			return 1;
		}

		if (!entry.IsFile()) {
			fprintf(stderr, "Error: \"%s\" is not a file.\n", path);
			return 1;
		}

		BPath normalizedPath;
		error = entry.GetPath(&normalizedPath);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to normalize path \"%s\": %s\n",
				path, strerror(error));
			return 1;
		}

		if (strlcpy(request.path, normalizedPath.Path(), sizeof(request.path))
				>= sizeof(request.path)) {
			fprintf(stderr, "Error: Normalized path too long.\n");
			return 1;
		}
	}

	status_t error = execute_control_device_ioctl(RAM_DISK_IOCTL_REGISTER,
		&request);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to create RAM disk device: %s\n",
			strerror(error));
		return 1;
	}

	printf("RAM disk device created as \"%s/%" B_PRId32 "/raw\"\n",
		kRamDiskRawDeviceBasePath, request.id);
	return 0;
}


static int
command_unregister(int argc, const char* const* argv)
{
	sCommandUsage = kDeleteUsage;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining argument is the device ID.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* idString = argv[optind++];
	char* end;
	long long id = strtol(idString, &end, 0);
	if (end == idString || *end != '\0' || id < 0 || id > INT32_MAX) {
		fprintf(stderr, "Error: Invalid ID \"%s\".\n", idString);
		return 1;
	}

	// check whether the raw device for that ID exists
	BString path;
	path.SetToFormat("%s/%s/raw", kRamDiskRawDeviceBasePath, idString);
	struct stat st;
	if (lstat(path, &st) != 0) {
		fprintf(stderr, "Error: No RAM disk with ID %s.\n", idString);
		return 1;
	}

	// issue the request
	ram_disk_ioctl_unregister request;
	request.id = (int32)id;

	status_t error = execute_control_device_ioctl(RAM_DISK_IOCTL_UNREGISTER,
		&request);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to delete RAM disk device: %s\n",
			strerror(error));
		return 1;
	}

	return 0;
}


static int
command_flush(int argc, const char* const* argv)
{
	sCommandUsage = kFlushUsage;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining argument is the device ID.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* idString = argv[optind++];
	char* end;
	long long id = strtol(idString, &end, 0);
	if (end == idString || *end != '\0' || id < 0 || id > INT32_MAX) {
		fprintf(stderr, "Error: Invalid ID \"%s\".\n", idString);
		return 1;
	}

	// open the raw device
	BString path;
	path.SetToFormat("%s/%s/raw", kRamDiskRawDeviceBasePath, idString);
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error: Failed to open RAM disk device \"%s\"\n",
			path.String());
		return 1;
	}
	FileDescriptorCloser fdCloser(fd);

	// issue the request
	if (ioctl(fd, RAM_DISK_IOCTL_FLUSH, NULL) < 0) {
		fprintf(stderr, "Error: Failed to flush RAM disk device: %s\n",
			strerror(errno));
		return 1;
	}

	return 0;
}


static int
command_list(int argc, const char* const* argv)
{
	sCommandUsage = kListUsage;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// There shouldn't be any remaining arguments.
	if (optind != argc)
		print_usage_and_exit(true);

	// iterate through the RAM disk device directory and search for raw devices
	DIR* dir = opendir(kRamDiskRawDeviceBasePath);
	if (dir == NULL) {
		fprintf(stderr, "Error: Failed to open RAM disk device directory: %s\n",
			strerror(errno));
		return 1;
	}
	CObjectDeleter<DIR, int> dirCloser(dir, &closedir);

	TextTable table;
	table.AddColumn("ID", B_ALIGN_RIGHT);
	table.AddColumn("Size", B_ALIGN_RIGHT);
	table.AddColumn("Associated file");

	while (dirent* entry = readdir(dir)) {
		// check, if the entry name could be an ID
		const char* idString = entry->d_name;
		char* end;
		long long id = strtol(idString, &end, 0);
		if (end == idString || *end != '\0' || id < 0 || id > INT32_MAX)
			continue;

		// open the raw device
		BString path;
		path.SetToFormat("%s/%s/raw", kRamDiskRawDeviceBasePath, idString);
		int fd = open(path, O_RDONLY);
		if (fd < 0)
			continue;
		FileDescriptorCloser fdCloser(fd);

		// issue the request
		ram_disk_ioctl_info request;
		if (ioctl(fd, RAM_DISK_IOCTL_INFO, &request) < 0)
			continue;

		int32 rowIndex = table.CountRows();
		table.SetTextAt(rowIndex, 0, BString() << request.id);
		table.SetTextAt(rowIndex, 1, BString() << request.size);
		table.SetTextAt(rowIndex, 2, request.path);
	}

	if (table.CountRows() > 0)
		table.Print(INT32_MAX);
	else
		printf("No RAM disks.\n");

	return 0;
}


int
main(int argc, const char* const* argv)
{
	if (argc < 2)
		print_usage_and_exit(true);

	if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0
		|| strcmp(argv[1], "-h") == 0) {
		print_usage_and_exit(false);
	}

	sCommandName = argv[1];

	if (strcmp(sCommandName, "create") == 0)
		return command_register(argc - 1, argv + 1);
	if (strcmp(sCommandName, "delete") == 0)
		return command_unregister(argc - 1, argv + 1);
	if (strcmp(sCommandName, "flush") == 0)
		return command_flush(argc - 1, argv + 1);
	if (strcmp(sCommandName, "list") == 0)
		return command_list(argc - 1, argv + 1);

	print_usage_and_exit(true);
}
