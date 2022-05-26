//	df - for Haiku
//
//	authors, in order of contribution:
//	jonas.sundstrom@kirilla.com
//	axeld@pinc-software.de
//


#include <Volume.h>
#include <Directory.h>
#include <Path.h>

#include <fs_info.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


void
PrintFlag(uint32 deviceFlags, uint32 testFlag, const char *yes, const char *no)
{
	printf("%s", deviceFlags & testFlag ? yes : no);
}


void
PrintMountPoint(dev_t device, bool verbose)
{
	char mount[B_PATH_NAME_LENGTH];
	mount[0] = '\0';

	BVolume	volume(device);
	BDirectory root;
	if (volume.GetRootDirectory(&root) == B_OK) {
		BPath path(&root, NULL);
		if (path.InitCheck() == B_OK)
			strlcpy(mount, path.Path(), sizeof(mount));
		else
			strlcpy(mount, "?", sizeof(mount));
	}

	if (verbose)
		printf("   Mounted at: %s\n", mount);
	else {
		printf("%-17s ", mount);
		if (strlen(mount) > 17)
			printf("\n%17s ", "");
	}
}


void
PrintType(const char *fileSystem)
{
	char type[16];
	strlcpy(type, fileSystem, sizeof(type));

	printf("%-9s", type);
}


const char *
ByteString(int64 numBlocks, int64 blockSize)
{
	double blocks = 1. * numBlocks * blockSize;
	static char string[64];

	if (blocks < 1024)
		sprintf(string, "%" B_PRId64, numBlocks * blockSize);
	else {
		const char *units[] = {"KiB", "MiB", "GiB", "TiB", "PiB", "EiB",
							   "ZiB", "YiB", NULL};
		int32 i = -1;

		do {
			blocks /= 1024.0;
			i++;
		} while (blocks >= 1024 && units[i + 1]);

		sprintf(string, "%.1f %s", blocks, units[i]);
	}

	return string;
}


void
PrintBlocks(int64 blocks, int64 blockSize, bool showBlocks)
{
	char temp[1024];

	if (showBlocks)
		sprintf(temp, "%" B_PRId64, blocks * (blockSize / 1024));
	else
		strcpy(temp, ByteString(blocks, blockSize));

	printf("%10s", temp);
}


void
PrintVerbose(dev_t device)
{
	fs_info info;
	if (fs_stat_dev(device, &info) != B_OK) {
		fprintf(stderr, "Could not stat fs: %s\n", strerror(errno));
		return;
	}

	printf("   Device No.: %" B_PRIdDEV "\n", info.dev);
	PrintMountPoint(info.dev, true);
	printf("  Volume Name: \"%s\"\n", info.volume_name);
	printf("  File System: %s\n", info.fsh_name);
	printf("       Device: %s\n", info.device_name);

	printf("        Flags: ");
	PrintFlag(info.flags, B_FS_HAS_QUERY, "Q", "-");
	PrintFlag(info.flags, B_FS_HAS_ATTR, "A", "-");
	PrintFlag(info.flags, B_FS_HAS_MIME, "M", "-");
	PrintFlag(info.flags, B_FS_IS_SHARED, "S", "-");
	PrintFlag(info.flags, B_FS_IS_PERSISTENT, "P", "-");
	PrintFlag(info.flags, B_FS_IS_REMOVABLE, "R", "-");
	PrintFlag(info.flags, B_FS_IS_READONLY, "-", "W");

	printf("\n     I/O Size: %10s (%" B_PRIdOFF " byte)\n",
		ByteString(info.io_size, 1), info.io_size);
	printf("   Block Size: %10s (%" B_PRIdOFF " byte)\n",
		ByteString(info.block_size, 1), info.block_size);
	printf(" Total Blocks: %10s (%" B_PRIdOFF " blocks)\n",
		ByteString(info.total_blocks, info.block_size), info.total_blocks);
	printf("  Free Blocks: %10s (%" B_PRIdOFF " blocks)\n",
		ByteString(info.free_blocks, info.block_size), info.free_blocks);
	printf("  Total Nodes: %" B_PRIdOFF "\n", info.total_nodes);
	printf("   Free Nodes: %" B_PRIdOFF "\n", info.free_nodes);
	printf("   Root Inode: %" B_PRIdINO "\n", info.root);
}


void
PrintCompact(dev_t device, bool showBlocks, bool all)
{
	fs_info info;
	if (fs_stat_dev(device, &info) != B_OK)
		return;

	if (!all && (info.flags & B_FS_IS_PERSISTENT) == 0)
		return;

	PrintMountPoint(info.dev, false);
	PrintType(info.fsh_name);
	PrintBlocks(info.total_blocks, info.block_size, showBlocks);
	PrintBlocks(info.free_blocks, info.block_size, showBlocks);

	printf(" ");
	PrintFlag(info.flags, B_FS_HAS_QUERY, "Q", "-");
	PrintFlag(info.flags, B_FS_HAS_ATTR, "A", "-");
	PrintFlag(info.flags, B_FS_HAS_MIME, "M", "-");
	PrintFlag(info.flags, B_FS_IS_SHARED, "S", "-");
	PrintFlag(info.flags, B_FS_IS_PERSISTENT, "P", "-");
	PrintFlag(info.flags, B_FS_IS_REMOVABLE, "R", "-");
	PrintFlag(info.flags, B_FS_IS_READONLY, "-", "W");

	printf(" %s\n", info.device_name);
}


void
ShowUsage(const char *programName)
{
	printf("usage: %s [--help | --blocks, -b | -all, -a] [<path-to-device>]\n"
		"  -a, --all\tinclude all file systems, also those not visible from Tracker\n"
		"  -b, --blocks\tshow device size in blocks of 1024 bytes\n"
		"If <path-to-device> is used, detailed info for that device only will be listed.\n"
		"Flags:\n"
		"   Q: has query\n"
		"   A: has attribute\n"
		"   M: has mime\n"
		"   S: is shared\n"
		"   P: is persistent (visible in Tracker)\n"
		"   R: is removable\n"
		"   W: is writable\n", programName);
	exit(0);
}


int
main(int argc, char **argv)
{
	char *programName = argv[0];
	if (strrchr(programName, '/'))
		programName = strrchr(programName, '/') + 1;

	bool showBlocks = false;
	bool all = false;
	dev_t device = -1;

	while (*++argv) {
		char *arg = *argv;
		if (*arg == '-') {
			while (*++arg && isalpha(*arg)) {
				switch (arg[0]) {
					case 'a':
						all = true;
						break;
					case 'b':
						showBlocks = true;
						break;
					case 'h':
						// human readable units in Unix df
						break;
					default:
						ShowUsage(programName);
				}
			}
			if (arg[0] == '-') {
				arg++;
				if (!strcmp(arg, "all"))
					all = true;
				else if (!strcmp(arg, "blocks"))
					showBlocks = true;
				else
					ShowUsage(programName);
			}
		} else
			break;
	}

	// Do we already have a device? Then let's print out detailed info about that

	if (argv[0] != NULL) {
		PrintVerbose(dev_for_path(argv[0]));
		return 0;
	}

	// If not, then just iterate over all devices and give a compact summary

	printf(" Mount             Type      Total     Free      Flags   Device\n"
		   "----------------- --------- --------- --------- ------- ------------------------\n");

	int32 cookie = 0;
	while ((device = next_dev(&cookie)) >= B_OK) {
		PrintCompact(device, showBlocks, all);
	}

	return 0;
}
