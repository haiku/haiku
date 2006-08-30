#include "bfs_control.h"

#include <fs_info.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>


const char *gProgramName;
int gDevice = -1;
uint32 gErrors;


static int
openDevice(const char *name)
{
	dev_t volume = dev_for_path(name);
	if (volume < B_OK) {
		fprintf(stderr, "%s: could not open \"%s\": %s\n", gProgramName, name, strerror(volume));
		return volume;
	}

	// check if it's a BFS volume

	fs_info info;
	if (fs_stat_dev(volume, &info) < B_OK) {
		fprintf(stderr, "%s: could not stat device: %s: %s\n", gProgramName, name, strerror(errno));
		return errno;
	}

	// ToDo: we can (and should) remove the "obfs" name later
	if (strcmp(info.fsh_name, "bfs") && strcmp(info.fsh_name, "obfs")) {
		fprintf(stderr, "%s: not a BFS volume\n", gProgramName);
		return B_BAD_TYPE;
	}

	int device = open(name, O_RDONLY);
	if (device < 0)
		fprintf(stderr, "%s: could not open \"%s\": %s\n", gProgramName, name, strerror(errno));
	else {
		uint32 version;
		if (ioctl(device, BFS_IOCTL_VERSION, &version) < B_OK) {
			fprintf(stderr, "%s: wrong BFS file system handler\n", gProgramName);
			close(device);
			return B_BAD_TYPE;
		}
		if (version < 0x0001000) {
			fprintf(stderr, "%s: wrong version of the BFS handler\n", gProgramName);
			close(device);
			return B_BAD_TYPE;
		}
	}

	return device;
}


static int
startChecking(int device, check_control &control)
{
	return ioctl(device, BFS_IOCTL_START_CHECKING, &control, sizeof(check_control));
}


static int
stopChecking(int device, check_control &control)
{
	int status = ioctl(device, BFS_IOCTL_STOP_CHECKING, &control, sizeof(check_control));
	if (status < B_OK)
	    printf("%s: error stopping!\n", gProgramName);

	return status;
}


static int
checkNext(int device, check_control &control)
{
	return ioctl(device, BFS_IOCTL_CHECK_NEXT_NODE, &control, sizeof(check_control));
}


static void
printError(const char *string, int32 &cookie)
{
	if (cookie++ != 0)
		printf(", ");

	printf(string);
}


static void
printUsage()
{
	printf("usage: %s [-nfe] <path-to-volume>\n"
		"\t-n\tdo not modify anything, just check\n"
		"\t-f\tdon't do anything right now, provided just for compatibility\n"
		"\t-e\tfix other fixable errors\n",
		gProgramName);
}


int
main(int argc, char **argv)
{
	struct check_control control;
	memset(&control, 0, sizeof(control));

	gProgramName = strrchr(argv[0], '/');
	if (gProgramName == NULL)
		gProgramName = argv[0];
	else
		gProgramName++;

	if (argc < 2 || !strcmp(argv[1], "--help"))
	{
		printUsage();
		return 1;
	}

	control.magic = BFS_IOCTL_CHECK_MAGIC;
	control.flags = BFS_FIX_BITMAP_ERRORS;

	while (*++argv)
	{
		char *arg = *argv;
		if (*arg == '-')
		{
			while (*++arg && isalpha(*arg))
			{
				switch (*arg)
				{
					case 'n':
						control.flags = 0;
						break;
					case 'f':
						// whatever...
						break;
					case 'e':
						control.flags |= BFS_REMOVE_WRONG_TYPES | BFS_REMOVE_INVALID;
						break;
				}
			}
		}
		else
			break;
	}

	int device = openDevice(argv[0]);
	if (device < 0)
	    return 1;

	// start checking

	if (startChecking(device, control) < B_OK) {
		close(device);
		return 1;
	}

	// reset all counters
	off_t nodes = 0, files = 0, directories = 0, indices = 0;
	off_t attributeDirectories = 0, attributes = 0;

	// check all files and report errors
	while (checkNext(device, control) == B_OK) {
		if (++nodes % 50 == 0)
			printf("  %7Ld nodes processed\x1b[1A\n", nodes);

		if (control.errors) {
			int32 cookie = 0;
			printf("%s (inode = %Ld): ", control.name, control.inode);

			gErrors |= control.errors;

			if (control.errors & BFS_MISSING_BLOCKS)
				printError("some blocks weren't allocated", cookie);
			if (control.errors & BFS_BLOCKS_ALREADY_SET)
				printError("has blocks already set", cookie);
			if (control.errors & BFS_INVALID_BLOCK_RUN)
				printError("has invalid block run(s)", cookie);
			if (control.errors & BFS_COULD_NOT_OPEN)
				printError("could not be opened", cookie);
			if (control.errors & BFS_WRONG_TYPE)
				printError("has wrong type", cookie);
			if (control.errors & BFS_NAMES_DONT_MATCH)
				printError("names don't match", cookie);
			putchar('\n');
		}

		// update counters
		if ((control.mode & (S_INDEX_DIR | 0777)) == S_INDEX_DIR)
			indices++;
		else if (control.mode & S_ATTR_DIR)
			attributeDirectories++;
		else if (control.mode & S_ATTR)
			attributes++;
		else if (control.mode & S_IFDIR)
			directories++;
		else
			files++;
	}
	if (control.status != B_ENTRY_NOT_FOUND)
		printf("%s: error occured during scan: %s\n", gProgramName, strerror(control.status));

	if (stopChecking(device, control) < B_OK) {
		close(device);
		return 1;
	}

	// print stats
	printf("checked %Ld nodes, %Ld blocks not allocated, %Ld blocks already set, %Ld blocks could be freed\n",
		nodes, control.stats.missing, control.stats.already_set, control.stats.freed);
	printf("\tfiles\t\t%Ld\n\tdirectories\t%Ld\n\tattributes\t%Ld\n\tattr. dirs\t%Ld\n\tindices\t\t%Ld\n",
		files, directories, attributes, attributeDirectories, indices);

	// print some more information
	if ((control.stats.missing > 0 || control.stats.freed > 0)
		&& (control.flags & BFS_FIX_BITMAP_ERRORS) == 0)
		printf("There were errors in the block bitmap - these will get fixed if you don't specify the -n option\n");
	if (gErrors & BFS_BLOCKS_ALREADY_SET)
		printf("Sorry, can't fix the \"blocks already set\" error - two files are claiming the same space\n");
	if ((gErrors & BFS_WRONG_TYPE) != 0 && (control.flags & BFS_REMOVE_WRONG_TYPES) == 0)
		printf("The wrong type errors can be fixed by specifying the -e option\n");
	if ((gErrors & BFS_COULD_NOT_OPEN) != 0 && (control.flags & BFS_REMOVE_INVALID) == 0)
		printf("Nodes that can't be opened will be removed when the -e option is specified\n");

	if (control.status == B_ENTRY_NOT_FOUND
		&&	((control.stats.missing > 0 || control.stats.freed > 0)
				&& (control.flags & BFS_FIX_BITMAP_ERRORS) != 0
			|| (gErrors & (BFS_WRONG_TYPE | BFS_COULD_NOT_OPEN)) != 0
				&& (control.flags & (BFS_REMOVE_WRONG_TYPES | BFS_REMOVE_INVALID))))
		printf("Errors have been fixed.\n");

	close(device);
	return 0;
}
