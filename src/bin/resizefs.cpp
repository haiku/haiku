/*
 * Copyright 2012, Andreas Henriksson, sausageboy@gmail.com.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskSystem.h>
#include <StringForSize.h>


extern "C" const char* __progname;
static const char* kProgramName = __progname;


int
main(int argc, char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <device> <size>\n"
			"Resize volume on <device> to new size <size> (in bytes, suffixes "
			"'k', 'm', 'g', 't' are interpreted as KiB, Mib, GiB, TiB).\n",
			kProgramName);
		return 1;
	}

	int64 size = parse_size(argv[2]);
	if (size <= 0) {
		fprintf(stderr, "%s: The new size \"%s\" is not a number\n",
			kProgramName, argv[2]);
		return 1;
	}

	BDiskDeviceRoster roster;
	BPartition* partition;
	BDiskDevice device;

	status_t status = roster.GetPartitionForPath(argv[1], &device,
		&partition);
	if (status != B_OK) {
		if (strncmp(argv[1], "/dev", 4)) {
			// try mounted volume
			status = roster.FindPartitionByMountPoint(argv[1], &device,
				&partition) ? B_OK : B_BAD_VALUE;
		}

		// TODO: try to register file device

		if (status != B_OK) {
			fprintf(stderr, "%s: Failed to get disk device for path \"%s\": "
				"%s\n", kProgramName, argv[1], strerror(status));
			return 1;
		}
	}

	// Prepare the device for modifications

	status = device.PrepareModifications();
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not prepare the device for modifications: "
			"%s\n", kProgramName, strerror(status));
		return false;
	}

	// Check if the partition supports resizing

	bool canResizeWhileMounted;
	bool canResize = partition->CanResize(NULL, &canResizeWhileMounted);

	if (!canResize && !canResizeWhileMounted) {
		fprintf(stderr, "%s: The disk system does not support resizing.\n",
			kProgramName);
		return 1;
	}

	if (partition->IsMounted() && !canResizeWhileMounted) {
		fprintf(stderr, "%s: The disk system does not support resizing a "
			"mounted volume.\n", kProgramName);
		return 1;
	}
	if (!partition->IsMounted() && !canResize) {
		fprintf(stderr, "%s: The disk system does not support resizing a "
			"volume that is not mounted.\n", kProgramName);
		return 1;
	}

	// Validate the requested size

	off_t validatedSize = size;
	status = partition->ValidateResize(&validatedSize);
	if (status != B_OK) {
		fprintf(stderr, "%s: Failed to validate the size: %s\n", kProgramName,
			strerror(status));
		return 1;
	}

	if (validatedSize != size) {
		printf("The requested size (%" B_PRId64 " bytes) was not accepted.\n"
			"Resize to %" B_PRId64 " bytes instead? (y/N): ", size,
			(int64)validatedSize);

		int c = getchar();
		if (c != 'y')
			return 0;
	}

	// Resize the volume

	status = partition->Resize(validatedSize);
	if (status != B_OK) {
		fprintf(stderr, "%s: Resizing failed: %s\n", kProgramName,
			strerror(status));
		return 1;
	}

	status = device.CommitModifications();
	if (status != B_OK) {
		fprintf(stderr, "%s: Failed to commit modifications: %s\n",
			kProgramName, strerror(status));
		return 1;
	}

	return 0;
}
