/*
 * Copyright 2005, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef TAR_FS_H
#define TAR_FS_H


enum {
	BLOCK_SIZE	= 512,
};

struct tar_header {
	char	name[100];
	char	mode[8];
	char	uid[8];
	char	gid[8];
	char	size[12];
	char	modification_time[12];
	char	check_sum[8];
	char	type;
	char	linkname[100];
	char	magic[6];
	char	version[2];
	char	user_name[32];
	char	group_name[32];
	char	device_major[8];
	char	device_minor[8];
	char	prefix[155];
};

static const char *kTarHeaderMagic = "ustar";
static const char *kOldTarHeaderMagic = "ustar  ";

// the relevant entry types
enum {
	TAR_FILE		= '0',
	TAR_FILE2		= '\0',
	TAR_SYMLINK		= '2',
	TAR_DIRECTORY	= '5',
	TAR_LONG_NAME	= 'L',
};

#endif	// TAR_FS_H
