
#include "compat.h"

#include <stdio.h>
#include <StorageDefs.h>

#include "fsproto.h"
#include "kprotos.h"
#include "bfs_control.h"

#include "additional_commands.h"


static int
do_chkbfs(int argc, char **argv)
{
	struct check_control result;
	fs_off_t files = 0, directories = 0, indices = 0, attributeDirectories = 0, attributes = 0;
	int counter = 0;

	int fd = sys_open(1, -1, "/myfs/.", O_RDONLY, S_IFREG, 0);
	if (fd < 0) {
	    printf("chkbfs: error opening '.'\n");
	    return fd;
	}

	memset(&result, 0, sizeof(result));
	result.magic = BFS_IOCTL_CHECK_MAGIC;
	result.flags = argc > 1 ? BFS_FIX_BITMAP_ERRORS : 0;
	if (argc > 2) {
		printf("will fix any severe errors!\n");
		result.flags |= BFS_REMOVE_WRONG_TYPES | BFS_REMOVE_INVALID;
	}

	// start checking
	if ((sys_ioctl(1, fd, BFS_IOCTL_START_CHECKING, &result, sizeof(result))) < 0) {
	    printf("chkbfs: error starting!\n");
	}

	// check all files and report errors
	while (sys_ioctl(1, fd, BFS_IOCTL_CHECK_NEXT_NODE, &result, sizeof(result)) == FS_OK) {
		if (++counter % 50 == 0)
			printf("  %7d nodes processed\x1b[1A\n", counter);

		if (result.errors) {
			printf("%s (inode = %Ld)", result.name, result.inode);
			if (result.errors & BFS_MISSING_BLOCKS)
				printf(", some blocks weren't allocated");
			if (result.errors & BFS_BLOCKS_ALREADY_SET)
				printf(", has blocks already set");
			if (result.errors & BFS_INVALID_BLOCK_RUN)
				printf(", has invalid block run(s)");
			if (result.errors & BFS_COULD_NOT_OPEN)
				printf(", could not be opened");
			if (result.errors & BFS_WRONG_TYPE)
				printf(", has wrong type");
			if (result.errors & BFS_NAMES_DONT_MATCH)
				printf(", names don't match");
			putchar('\n');
		}
		if ((result.mode & (MY_S_INDEX_DIR | 0777)) == MY_S_INDEX_DIR)
			indices++;
		else if (result.mode & MY_S_ATTR_DIR)
			attributeDirectories++;
		else if (result.mode & MY_S_ATTR)
			attributes++;
		else if (result.mode & MY_S_IFDIR)
			directories++;
		else
			files++;
	}
	if (result.status != FS_ENTRY_NOT_FOUND)
		printf("chkbfs: error occured during scan: %s\n", strerror(result.status));

	// stop checking
	if ((sys_ioctl(1, fd, BFS_IOCTL_STOP_CHECKING, &result, sizeof(result))) < 0) {
	    printf("chkbfs: error stopping!\n");
	}

	printf("checked %d nodes, %Ld blocks not allocated, %Ld blocks already set, %Ld blocks could be freed\n",
		counter, result.stats.missing, result.stats.already_set, result.stats.freed);
	printf("\tfiles\t\t%Ld\n\tdirectories\t%Ld\n\tattributes\t%Ld\n\tattr. dirs\t%Ld\n\tindices\t\t%Ld\n",
		files, directories, attributes, attributeDirectories, indices);
	if (result.flags & BFS_FIX_BITMAP_ERRORS)
		printf("errors have been fixed\n");

	sys_close(1, fd);

	return 0;
}

cmd_entry additional_commands[] = {
	{ "chkbfs",	 do_chkbfs, "does a chkbfs on the volume" },
    { NULL, NULL }
};
