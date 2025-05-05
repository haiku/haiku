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


enum FormatType
{
	k512Block,
	k1024Block,
	kHumanReadable,
	kUnspecified
};


void
PrintFlag(uint32 deviceFlags, uint32 testFlag, char yes, char no)
{
	printf("%c", deviceFlags & testFlag ? yes : no);
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

	printf("%11s ", type);
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


const char *
UsePercentage(int64 totalBlocks, int64 freeBlocks, bool compact = false)
{
	float usage;
	static char string[8];

	if (totalBlocks == 0 && freeBlocks == 0)
		usage = 100;
	else if (totalBlocks == 0)
		usage = 0;
	else
		usage = 100 - 100 * (1. * freeBlocks / totalBlocks);

	if (compact) {
		if (usage < 10)
			sprintf(string, "  %.0f%%", round(usage));
		else if (usage < 100)
			sprintf(string, " %.0f%%", round(usage));
		else
			sprintf(string, "%.0f%%", round(usage));
	}
	else {
		if (usage < 10)
			sprintf(string, "  %.1f %%", usage);
		else if (usage < 100)
			sprintf(string, " %.1f %%", usage);
		else
			sprintf(string, "%.1f %%", usage);
	}

	return string;
}


void
PrintBlocks(int64 blocks, int64 blockSize, FormatType format)
{
	char temp[1024];

	if (format == k1024Block)
		sprintf(temp, "%" B_PRId64, blocks * (blockSize / 1024));
	else if (format == k512Block)
		sprintf(temp, "%" B_PRId64, blocks * (blockSize / 512));
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

	int64 usedBlocks = info.total_blocks - info.free_blocks;
	// Used blocks can't be negative, but ramfs reports available RAM as free_blocks,
	// and always 0 for total_blocks, so we need to check for that.
	if (usedBlocks < 0)
		usedBlocks = 0;

	printf("   Device No.: %" B_PRIdDEV "\n", info.dev);
	PrintMountPoint(info.dev, true);
	printf("  Volume Name: \"%s\"\n", info.volume_name);
	printf("  File System: %s\n", info.fsh_name);
	printf("       Device: %s\n", info.device_name);

	printf("        Flags: ");
	PrintFlag(info.flags, B_FS_HAS_QUERY, 'Q', '-');
	PrintFlag(info.flags, B_FS_HAS_ATTR, 'A', '-');
	PrintFlag(info.flags, B_FS_HAS_MIME, 'M', '-');
	PrintFlag(info.flags, B_FS_IS_SHARED, 'S', '-');
	PrintFlag(info.flags, B_FS_IS_PERSISTENT, 'P', '-');
	PrintFlag(info.flags, B_FS_IS_REMOVABLE, 'R', '-');
	PrintFlag(info.flags, B_FS_IS_READONLY, '-', 'W');

	printf("\n     I/O Size: %10s (%" B_PRIdOFF " byte)\n",
		ByteString(info.io_size, 1), info.io_size);
	printf("   Block Size: %10s (%" B_PRIdOFF " byte)\n",
		ByteString(info.block_size, 1), info.block_size);
	printf(" Total Blocks: %10s (%" B_PRIdOFF " blocks)\n",
		ByteString(info.total_blocks, info.block_size), info.total_blocks);
	printf("  Used Blocks: %10s (%" B_PRIdOFF " blocks)\n",
		ByteString(usedBlocks, info.block_size), usedBlocks);
	printf("  Free Blocks: %10s (%" B_PRIdOFF " blocks)\n",
		ByteString(info.free_blocks, info.block_size), info.free_blocks);
	printf("      Usage %%:  %s\n", UsePercentage(info.total_blocks, info.free_blocks));
	printf("  Total Nodes: %" B_PRIdOFF "\n", info.total_nodes);
	printf("   Used Nodes: %" B_PRIdOFF "\n", info.total_nodes - info.free_nodes);
	printf("   Free Nodes: %" B_PRIdOFF "\n", info.free_nodes);
	printf("   Root Inode: %" B_PRIdINO "\n", info.root);
}


void
PrintCompact(dev_t device, FormatType format, bool strictPosix, bool all)
{
	fs_info info;
	if (fs_stat_dev(device, &info) != B_OK)
		return;

	if (!all && (info.flags & B_FS_IS_PERSISTENT) == 0)
		return;

	int64 usedBlocks = info.total_blocks - info.free_blocks;
	// Used blocks can't be negative, but ramfs reports available RAM as free_blocks,
	// and always 0 for total_blocks, so we need to check for that.
	if (usedBlocks < 0)
		usedBlocks = 0;

	if (strictPosix) {
		printf("%24s ", info.device_name);
		PrintBlocks(info.total_blocks, info.block_size, format);
		PrintBlocks(usedBlocks, info.block_size, format);
		PrintBlocks(info.free_blocks, info.block_size, format);
		printf("     %s ", UsePercentage(info.total_blocks, info.free_blocks, true));
		PrintMountPoint(info.dev, false);
	} else {
		printf("\x1B[1m");
		PrintMountPoint(info.dev, false);
		printf("\x1B[0m\n");
		PrintType(info.fsh_name);
		PrintBlocks(info.total_blocks, info.block_size, format);
		PrintBlocks(usedBlocks, info.block_size, format);

		printf(" ");
		PrintFlag(info.flags, B_FS_HAS_QUERY, 'Q', '-');
		PrintFlag(info.flags, B_FS_HAS_ATTR, 'A', '-');
		PrintFlag(info.flags, B_FS_HAS_MIME, 'M', '-');
		PrintFlag(info.flags, B_FS_IS_SHARED, 'S', '-');
		PrintFlag(info.flags, B_FS_IS_PERSISTENT, 'P', '-');
		PrintFlag(info.flags, B_FS_IS_REMOVABLE, 'R', '-');
		PrintFlag(info.flags, B_FS_IS_READONLY, '-', 'W');

		printf(" %6s ", UsePercentage(info.total_blocks, info.free_blocks, true));
		printf("%-24s", info.device_name);
	}
	printf("\n");
}


void
ShowUsage(const char *programName)
{
	printf("usage: %s [--help | --blocks, -b | -all, -a] [<path-to-volume>]\n"
		"  -a, --all\tinclude all file systems, also those not visible from Tracker\n"
		"  -b, --blocks\tshow device size in blocks of 512 bytes\n"
		"  -h, --human\tshow device size in blocks of 1024 bytes\n"
		"  -k, --kilobytes\tshow device size in blocks of 1024 bytes\n"
		"  -P, --posix\tproduce output in the format specified by POSIX\n"
		"If <path-to-volume> is used, detailed info for that volume only will be listed.\n"
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

	FormatType format = kUnspecified;
	bool strictPosix = false;
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
						format = k512Block;
						break;
					case 'h':
						// human readable units in Unix df
						format = kHumanReadable;
						break;
					case 'k':
						format = k1024Block;
						break;
					case 'P':
						strictPosix = true;
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
					format = k512Block;
				else if (!strcmp(arg, "human"))
					format = kHumanReadable;
				else if (!strcmp(arg, "kilobytes"))
					format = k1024Block;
				else if (!strcmp(arg, "posix"))
					strictPosix = true;
				else
					ShowUsage(programName);
			}
		} else
			break;
	}

	// Determine default format if nothing was specified
	// This is done after option parsing so that -k -P gives kilobytes as expected
	// Otherwise -P could force 512-block format, but it would overwrite the effect of other
	// options.
	if (format == kUnspecified) {
		if (strictPosix)
			format = k512Block;
		else
			format = kHumanReadable;
	}

	// Do we already have a device? Then let's print out detailed info about that

	if (argv[0] != NULL) {
		PrintVerbose(dev_for_path(argv[0]));
		return 0;
	}

	// If not, then just iterate over all devices and give a compact summary

	if (strictPosix) {
		printf(" Filesystem             %11s Used      Available Capacity Mounted on\n",
			format == kHumanReadable ? "Capacity"
				: (format == k512Block ? "512-blocks" : "1024-blocks"));
	} else {
		printf("\x1B[1mVolume\x1B[0m\n"
			   " Type        Total      Free      Flags   Used%% Device\n"
			   "----------- ---------- --------- ------- ------ -------------------\n");
	}

	int32 cookie = 0;
	while ((device = next_dev(&cookie)) >= B_OK) {
		PrintCompact(device, format, strictPosix, all);
	}

	return 0;
}
