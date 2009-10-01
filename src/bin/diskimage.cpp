/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Path.h>


extern const char* __progname;
const char* kCommandName = __progname;


static const char* kUsage =
	"Usage: %s register <file>\n"
	"       %s unregister ( <file> | <device path> | <device ID> )\n"
	"       %s list\n"
	"Registers a regular file as disk device, unregisters an already "
		"registered\n"
	"one, or lists all file disk devices.\n"
	"\n"
	"Options:\n"
	"  -h, --help   - Print this usage info.\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kCommandName, kCommandName,
		kCommandName);
    exit(error ? 1 : 0);
}


static status_t
list_file_disk_devices()
{
	printf("    ID  File                               Device\n");
	printf("------------------------------------------------------------------"
		"-------------\n");

	BDiskDeviceRoster roster;
	BDiskDevice device;
	while (roster.GetNextDevice(&device) == B_OK) {
		if (!device.IsFile())
			continue;

		// ID
		printf("%6ld  ", device.ID());

		// file path
		BPath path;
		printf("%-35s",
			device.GetFilePath(&path) == B_OK ? path.Path() : "???");

		// device path
		printf("%s", device.GetPath(&path) == B_OK ? path.Path() : "???");
		printf("\n");
	}

	return B_OK;
}


static status_t
register_file_disk_device(const char* fileName)
{
	// stat() the file to verify that it's a regular file
	struct stat st;
	if (lstat(fileName, &st) != 0) {
		status_t error = errno;
		fprintf(stderr, "Error: Failed to stat() \"%s\": %s\n", fileName,
			strerror(error));
		return error;
	}

	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "Error: \"%s\" is not a regular file.\n", fileName);
		return B_BAD_VALUE;
	}

	// register the file
	BDiskDeviceRoster roster;
	partition_id id = roster.RegisterFileDevice(fileName);
	if (id < 0) {
		fprintf(stderr, "Error: Failed to register file disk device: %s\n",
			strerror(id));
		return id;
	}

	// print the success message (get the device path)
	BDiskDevice device;
	BPath path;
	if (roster.GetDeviceWithID(id, &device) == B_OK
		&& device.GetPath(&path) == B_OK) {
		printf("Registered file as disk device \"%s\" with ID %ld.\n",
			path.Path(), id);
	} else {
		printf("Registered file as disk device with ID %ld, but failed to "
			"get the device path.\n", id);
	}

	return B_OK;
}


static status_t
unregister_file_disk_device(const char* fileNameOrID)
{
	// try to parse the parameter as ID
	char* numberEnd;
	partition_id id = strtol(fileNameOrID, &numberEnd, 0);
	BDiskDeviceRoster roster;
	if (id >= 0 && numberEnd != fileNameOrID && *numberEnd == '\0') {
		BDiskDevice device;
		if (roster.GetDeviceWithID(id, &device) == B_OK && device.IsFile()) {
			status_t error = roster.UnregisterFileDevice(id);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to unregister file disk device "
					"with ID %ld: %s\n", id, strerror(error));
				return error;
			}

			printf("Unregistered file disk device with ID %ld.\n", id);
			return B_OK;
		} else {
			fprintf(stderr, "No file disk device with ID %ld, trying file "
				"\"%s\"\n", id, fileNameOrID);
		}
	}

	// the parameter must be a file name -- stat() it
	struct stat st;
	if (lstat(fileNameOrID, &st) != 0) {
		status_t error = errno;
		fprintf(stderr, "Error: Failed to stat() \"%s\": %s\n", fileNameOrID,
			strerror(error));
		return error;
	}

	// remember the volume and node ID, so we can identify the file
	// NOTE: There's a race condition -- we would need to open the file and
	// keep it open to avoid it.
	dev_t volumeID = st.st_dev;
	ino_t nodeID = st.st_ino;

	// iterate through all file disk devices and try to find a match
	BDiskDevice device;
	while (roster.GetNextDevice(&device) == B_OK) {
		if (!device.IsFile())
			continue;

		// get file path and stat it, same for the device path
		BPath path;
		bool isFilePath = true;
		if ((device.GetFilePath(&path) == B_OK && lstat(path.Path(), &st) == 0
				&& volumeID == st.st_dev && nodeID == st.st_ino)
			|| (isFilePath = false, false)
			|| (device.GetPath(&path) == B_OK && lstat(path.Path(), &st) == 0
				&& volumeID == st.st_dev && nodeID == st.st_ino)) {
			status_t error = roster.UnregisterFileDevice(device.ID());
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to unregister file disk device"
					"%s \"%s\" (ID: %ld): %s\n", isFilePath ? " for file" : "",
					fileNameOrID, device.ID(), strerror(error));
				return error;
			}

			printf("Unregistered file disk device%s \"%s\" (ID: %ld)\n",
				isFilePath ? " for file" : "", fileNameOrID, device.ID());

			return B_OK;
		}
	}

	fprintf(stderr, "Error: \"%s\" does not refer to a file disk device.\n",
		fileNameOrID);
	return B_BAD_VALUE;
}


int
main(int argc, const char* const* argv)
{
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

	// Of the remaining arguments the next one should be the command.
	if (optind >= argc)
		print_usage_and_exit(true);

	status_t error = B_OK;
	const char* command = argv[optind++];

	if (strcmp(command, "list") == 0) {
		if (optind < argc)
			print_usage_and_exit(true);

		list_file_disk_devices();
	} else if (strcmp(command, "register") == 0) {
		if (optind + 1 != argc)
			print_usage_and_exit(true);

		const char* fileName = argv[optind++];
		register_file_disk_device(fileName);
	} else if (strcmp(command, "unregister") == 0) {
		if (optind + 1 != argc)
			print_usage_and_exit(true);

		const char* fileName = argv[optind++];
		unregister_file_disk_device(fileName);
	} else
		print_usage_and_exit(true);

	return error == B_OK ? 0 : 1;
}
