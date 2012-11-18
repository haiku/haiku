/*
 * Copyright 2002-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundstrom, jonas.sundstrom@kirilla.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <fs_info.h>

#include <stdio.h>
#include <string.h>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Volume.h>


static void
usage()
{
	fprintf(stderr, 
		"Usage: isvolume {-OPTION} [volumename]\n"
		"   Where OPTION is one of:\n"
		"      -readonly         - volume is read-only\n"
		"      -readonly-partion - partition for the volume is read-only\n"
		"      -query            - volume supports queries\n"
		"      -attribute        - volume supports attributes\n"
		"      -mime             - volume supports MIME information\n"
		"      -shared           - volume is shared\n"
		"      -persistent       - volume is backed on permanent storage\n"
		"      -removable        - volume is on removable media\n"
		"   If the option is true for the named volume, 'yes' is printed\n"
		"   and if the option is false, 'no' is printed. Multiple options\n"
		"   can be specified in which case all of them must be true.\n\n"
		"   If no volume is specified, the volume of the current directory is "
		"assumed.\n");
}


int
main(int argc, char** argv)
{
	dev_t volumeDevice = dev_for_path(".");
	uint32 isVolumeFlags = 0;
	fs_info volumeInfo;
	bool doPartitionReadOnlyCheck = false;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--help")) {
			usage();
			return 0;
		}
	
		if (argv[i][0] == '-') {
			if (strcmp(argv[i], "-readonly") == 0)
				isVolumeFlags |= B_FS_IS_READONLY;
			else if (strcmp(argv[i], "-query") == 0)
				isVolumeFlags |= B_FS_HAS_QUERY;
			else if (strcmp(argv[i], "-attribute") == 0)
				isVolumeFlags |= B_FS_HAS_ATTR;
			else if (strcmp(argv[i], "-mime") == 0)
				isVolumeFlags |= B_FS_HAS_MIME;
			else if (strcmp(argv[i], "-shared") == 0)
				isVolumeFlags |= B_FS_IS_SHARED;
			else if (strcmp(argv[i], "-persistent") == 0)
				isVolumeFlags |= B_FS_IS_PERSISTENT;
			else if (strcmp(argv[i], "-removable") == 0)
				isVolumeFlags |= B_FS_IS_REMOVABLE;
			else if (strcmp(argv[i], "-readonly-partion"))
				doPartitionReadOnlyCheck = true;
			else {	
				fprintf(stderr,
					"%s: option %s is not understood (use --help for help)\n",
					argv[0], argv[i]);
				return 1;
			}
		} else {
			volumeDevice = dev_for_path(argv[i]);

			if (volumeDevice < 0) {
				fprintf(stderr, "%s: can't get information about volume: %s\n",
					argv[0], argv[i]);
				return 1;
			}
		}
	}

	if (doPartitionReadOnlyCheck) {
		// This requires an extra code-path, because volumes may now appear
		// writable, but only because of the "write" file-system overlay.
		BVolume volume(volumeDevice);
		status_t ret = volume.InitCheck();
		if (ret != B_OK) {
			fprintf(stderr, "%s: failed to get BVolume for device %" B_PRIdDEV
				": %s\n", argv[0], volumeDevice, strerror(ret));
			return 1;
		}

		BDiskDeviceRoster roster;
		BDiskDevice diskDevice;
		BPartition* partition;
		ret = roster.FindPartitionByVolume(volume, &diskDevice, &partition);
		if (ret != B_OK) {
			fprintf(stderr, "%s: failed to get partition for device %" B_PRIdDEV
				": %s\n", argv[0], volumeDevice, strerror(ret));
			return 1;
		}
		// check this option directly and not via fs_stat_dev()
		if (partition->IsReadOnly()) {
			printf("yes\n");
			return 0;
		}
	}

	if (fs_stat_dev(volumeDevice, &volumeInfo) == B_OK) {
		if (volumeInfo.flags & isVolumeFlags)
			printf("yes\n");
		else
			printf("no\n");

		return 0;
	} else {
		fprintf(stderr, "%s: can't get information about dev_t: %" B_PRIdDEV
			"\n", argv[0], volumeDevice);
		return 1;
	}
}
