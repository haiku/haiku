/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskSystem.h>


extern "C" const char* __progname;
static const char* kProgramName = __progname;


void
usage(FILE* output)
{
	fprintf(output,
		"Usage: %s <options> <device|volume name>\n"
		"\n"
		"Options:\n"
		"  -h, --help\t\t- print this help text\n"
		"  -c, --check-only\t- do not make any changes to the file system\n"
		"\n"
		"Examples:\n"
		"  %s -c /Haiku\n"
		"  %s /dev/disk/ata/0/master/raw\n",
		kProgramName, kProgramName, kProgramName);
}


int
main(int argc, char** argv)
{
	const struct option kLongOptions[] = {
		{ "help", 0, NULL, 'h' },
		{ "check-only", 0, NULL, 'c' },
		{ NULL, 0, NULL, 0 }
	};
	const char* kShortOptions = "hc";

	// parse argument list
	bool checkOnly = false;

	while (true) {
		int nextOption = getopt_long(argc, argv, kShortOptions, kLongOptions,
			NULL);
		if (nextOption == -1)
			break;

		switch (nextOption) {
			case 'h':	// --help
				usage(stdout);
				return 0;
			case 'c':	// --check-only
				checkOnly = true;
				break;
			default:	// everything else
				usage(stderr);
				return 1;
		}
	}

	// the device name should be the only non-option element
	if (optind != argc - 1) {
		usage(stderr);
		return 1;
	}

	const char* path = argv[optind];
	//UnregisterFileDevice unregisterFileDevice;

	BDiskDeviceRoster roster;
	BPartition* partition;
	BDiskDevice device;

	status_t status = roster.GetPartitionForPath(path, &device,
		&partition);
	if (status != B_OK) {
		if (strncmp(path, "/dev", 4)) {
			// try mounted volume
			status = roster.FindPartitionByMountPoint(path, &device, &partition)
				? B_OK : B_BAD_VALUE;
		}

		// TODO: try to register file device

		if (status != B_OK) {
			fprintf(stderr, "%s: Failed to get disk device for path \"%s\": "
				"%s\n", kProgramName, path, strerror(status));
			return 1;
		}
	}

	// Prepare the device for modifications

	status = device.PrepareModifications();
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not prepare the device for modifications: "
			"%s\n", kProgramName, strerror(status));
		return 1;
	}

	// Check if the partition supports repairing

	bool canRepairWhileMounted;
	bool canRepair = partition->CanRepair(checkOnly, &canRepairWhileMounted);
	if (!canRepair && !canRepairWhileMounted) {
		fprintf(stderr, "%s: The disk system does not support repairing.\n",
			kProgramName);
		return 1;
	}

	if (partition->IsMounted() && !canRepairWhileMounted) {
		fprintf(stderr, "%s: The disk system does not support repairing a "
			"mounted volume.\n", kProgramName);
		return 1;
	}
	if (!partition->IsMounted() && !canRepair) {
		fprintf(stderr, "%s: The disk system does not support repairing a "
			"volume that is not mounted.\n", kProgramName);
		return 1;
	}

	BDiskSystem diskSystem;
	status = partition->GetDiskSystem(&diskSystem);
	if (status != B_OK) {
		fprintf(stderr, "%s: Failed to get disk system for partition: %s\n",
			kProgramName, strerror(status));
		return 1;
	}

	// Repair the volume

	status = partition->Repair(checkOnly);
	if (status != B_OK) {
		fprintf(stderr, "%s: Repairing failed: %s\n", kProgramName,
			strerror(status));
		return 1;
	}

	status = device.CommitModifications();

	return 0;
}
