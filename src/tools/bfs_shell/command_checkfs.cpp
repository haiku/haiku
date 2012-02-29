/*
 * Copyright 2008-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "fssh_stdio.h"
#include "syscalls.h"

#include "bfs.h"
#include "bfs_control.h"


namespace FSShell {


fssh_status_t
command_checkfs(int argc, const char* const* argv)
{
	if (argc == 2 && !strcmp(argv[1], "--help")) {
		fssh_dprintf("Usage: %s [-c]\n"
			"  -c  Check only; don't perform any changes\n", argv[0]);
		return B_OK;
	}

	bool checkOnly = false;
	if (argc == 2 && !strcmp(argv[1], "-c"))
		checkOnly = true;

	int rootDir = _kern_open_dir(-1, "/myfs");
	if (rootDir < 0)
		return rootDir;

	struct check_control result;
	memset(&result, 0, sizeof(result));
	result.magic = BFS_IOCTL_CHECK_MAGIC;
	result.flags = 0;
	if (!checkOnly) {
		result.flags |= BFS_FIX_BITMAP_ERRORS | BFS_REMOVE_WRONG_TYPES
			| BFS_REMOVE_INVALID | BFS_FIX_NAME_MISMATCHES | BFS_FIX_BPLUSTREES;
	}

	// start checking
	fssh_status_t status = _kern_ioctl(rootDir, BFS_IOCTL_START_CHECKING,
		&result, sizeof(result));
	if (status != B_OK)
	    return status;

	uint64 attributeDirectories = 0, attributes = 0;
	uint64 files = 0, directories = 0, indices = 0;
	uint64 counter = 0;

	// check all files and report errors
	while (_kern_ioctl(rootDir, BFS_IOCTL_CHECK_NEXT_NODE, &result,
			sizeof(result)) == B_OK) {
		if (++counter % 50 == 0)
			fssh_dprintf("%9Ld nodes processed\x1b[1A\n", counter);

		if (result.errors) {
			fssh_dprintf("%s (inode = %lld)", result.name, result.inode);
			if ((result.errors & BFS_MISSING_BLOCKS) != 0)
				fssh_dprintf(", some blocks weren't allocated");
			if ((result.errors & BFS_BLOCKS_ALREADY_SET) != 0)
				fssh_dprintf(", has blocks already set");
			if ((result.errors & BFS_INVALID_BLOCK_RUN) != 0)
				fssh_dprintf(", has invalid block run(s)");
			if ((result.errors & BFS_COULD_NOT_OPEN) != 0)
				fssh_dprintf(", could not be opened");
			if ((result.errors & BFS_WRONG_TYPE) != 0)
				fssh_dprintf(", has wrong type");
			if ((result.errors & BFS_NAMES_DONT_MATCH) != 0)
				fssh_dprintf(", names don't match");
			if ((result.errors & BFS_INVALID_BPLUSTREE) != 0)
				fssh_dprintf(", invalid b+tree");
			fssh_dprintf("\n");
		}
		if ((result.mode & (S_INDEX_DIR | 0777)) == S_INDEX_DIR)
			indices++;
		else if (result.mode & S_ATTR_DIR)
			attributeDirectories++;
		else if (result.mode & S_ATTR)
			attributes++;
		else if (S_ISDIR(result.mode))
			directories++;
		else
			files++;
	}

	// stop checking
	if (_kern_ioctl(rootDir, BFS_IOCTL_STOP_CHECKING, &result, sizeof(result))
			!= B_OK) {
		_kern_close(rootDir);
		return errno;
	}

	_kern_close(rootDir);

	fssh_dprintf("\t%" B_PRIu64 " nodes checked,\n\t%" B_PRIu64 " blocks not "
		"allocated,\n\t%" B_PRIu64 " blocks already set,\n\t%" B_PRIu64
		" blocks could be freed\n\n", counter, result.stats.missing,
		result.stats.already_set, result.stats.freed);
	fssh_dprintf("\tfiles\t\t%" B_PRIu64 "\n\tdirectories\t%" B_PRIu64 "\n"
		"\tattributes\t%" B_PRIu64 "\n\tattr. dirs\t%" B_PRIu64 "\n"
		"\tindices\t\t%" B_PRIu64 "\n", files, directories, attributes,
		attributeDirectories, indices);

	fssh_dprintf("\n\tdirect block runs\t\t%" B_PRIu64 " (%lld)\n",
		result.stats.direct_block_runs,
		result.stats.blocks_in_direct * result.stats.block_size);
	fssh_dprintf("\tindirect block runs\t\t%" B_PRIu64 " (in %" B_PRIu64
		" array blocks, %lld)\n", result.stats.indirect_block_runs,
		result.stats.indirect_array_blocks,
		result.stats.blocks_in_indirect * result.stats.block_size);
	fssh_dprintf("\tdouble indirect block runs\t%" B_PRIu64 " (in %" B_PRIu64
		" array blocks, %lld)\n", result.stats.double_indirect_block_runs,
		result.stats.double_indirect_array_blocks,
		result.stats.blocks_in_double_indirect * result.stats.block_size);

	if (result.status == B_ENTRY_NOT_FOUND)
		result.status = B_OK;

	return result.status;
}


}	// namespace FSShell
