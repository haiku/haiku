/*
 * Copyright 2006-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <OS.h>
#include <fs_info.h>

#include <extended_system_info.h>
#include <extended_system_info_defs.h>
#include <syscalls.h>
#include <vfs_defs.h>
#include <util/KMessage.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum info_mode {
	kList,
	kFilterDevice,
	kFilterFile,
};

extern const char *__progname;


static const char *
open_mode_to_string(int openMode)
{
	switch (openMode & O_RWMASK) {
		case O_RDONLY:
			return "R  ";
		case O_WRONLY:
			return "  W";
		default:
			return "R/W";
	}
}


static status_t
get_next_fd_info(team_id teamID, int32* cookie, fd_info* info)
{
	while (*cookie < 0) {
		*cookie = 0;

		// Fetch CWD before regular FDs.
		KMessage teamInfo;
		status_t error = get_extended_team_info(teamID, B_TEAM_INFO_BASIC, teamInfo);
		if (error != B_OK)
			break;

		int32 cwdDevice = 0;
		int64 cwdDirectory = 0;
		if (teamInfo.FindInt32("cwd device", &cwdDevice) != B_OK
			|| teamInfo.FindInt64("cwd directory", &cwdDirectory) != B_OK) {
			break;
		}

		info->number = AT_FDCWD;
		info->device = cwdDevice;
		info->node = cwdDirectory;
		info->open_mode = 0;
		return B_OK;
	}

	return _kern_get_next_fd_info(teamID, (uint32*)cookie, info, sizeof(fd_info));
}


void
print_fds(team_info &teamInfo)
{
	printf("Team: (%" B_PRId32 ") %s\n", teamInfo.team, teamInfo.args);

	int32 cookie = -1;
	fd_info info;

	while (get_next_fd_info(teamInfo.team, &cookie, &info) == B_OK) {
		printf("%5d  %s %" B_PRIdDEV ":%" B_PRIdINO "\n", info.number,
			open_mode_to_string(info.open_mode), info.device, info.node);
	}
}


void
filter_device(team_info &teamInfo, dev_t device, bool brief)
{
	int32 cookie = -1;
	fd_info info;

	while (get_next_fd_info(teamInfo.team, &cookie, &info) == B_OK) {
		if (info.device != device)
			continue;

		if (brief) {
			printf("%5" B_PRId32 " %s\n", teamInfo.team, teamInfo.args);
			break;
		}

		printf("%5" B_PRId32 " %4d  %3s  %" B_PRIdDEV ":%" B_PRIdINO " %s\n",
			teamInfo.team, info.number, open_mode_to_string(info.open_mode),
				info.device, info.node, teamInfo.args);
	}
}


void
filter_file(team_info &teamInfo, dev_t device, ino_t node, bool brief)
{
	int32 cookie = -1;
	fd_info info;

	while (get_next_fd_info(teamInfo.team, &cookie, &info) == B_OK) {
		if (info.device != device || info.node != node)
			continue;

		if (brief) {
			printf("%5" B_PRId32 " %s\n", teamInfo.team, teamInfo.args);
			break;
		}

		printf("%5" B_PRId32 " %4d  %3s  %s\n", teamInfo.team, info.number,
			open_mode_to_string(info.open_mode), teamInfo.args);
	}
}


void
usage(bool failure)
{
	printf("Usage: %s <id/pattern> or -[dD] <path-to-device> or -[fF] <file>\n"
		"  Shows info about the used file descriptors in the system.\n\n"
		"  -d\tOnly shows accesses to the given device\n"
		"  -D\tLikewise, but only shows the teams that access it\n"
		"  -f\tOnly shows accesses to the given file\n"
		"  -F\tLikewise, but only shows the teams that access it\n",
		__progname);

	exit(failure ? 1 : 0);
}


int
main(int argc, char **argv)
{
	const char *pattern = NULL;
	dev_t device = -1;
	ino_t node = -1;
	int32 id = -1;
	info_mode mode = kList;
	bool brief = false;

	// parse arguments

	if (argc == 2) {
		// filter output
		if (isdigit(argv[1][0]))
			id = atol(argv[1]);
		else if (argv[1][0] == '-')
			usage(!strcmp(argv[1], "--help"));
		else
			pattern = argv[1];
	} else if (argc > 2) {
		if (!strcmp(argv[1], "-d") || !strcmp(argv[1], "-D")) {
			// filter by device usage
			device = dev_for_path(argv[2]);
			if (device < 0) {
				fprintf(stderr, "%s: could not find device: %s\n", __progname,
					strerror(errno));
				return 1;
			}
			mode = kFilterDevice;
			if (argv[1][1] == 'D')
				brief = true;
		} else if (!strcmp(argv[1], "-f") || !strcmp(argv[1], "-F")) {
			// filter by file usage
			struct stat stat;
			if (::stat(argv[2], &stat) < 0) {
				fprintf(stderr, "%s: could not open file: %s\n", __progname,
					strerror(errno));
				return 1;
			}
			device = stat.st_dev;
			node = stat.st_ino;
			mode = kFilterFile;
			if (argv[1][1] == 'F')
				brief = true;
		} else
			usage(true);
	}

	// do the job!

	team_info info;
	int32 cookie = 0;

	while (get_next_team_info(&cookie, &info) == B_OK) {
		switch (mode) {
			case kList:
				if ((id != -1 && id != info.team)
					|| (pattern != NULL && !strstr(info.args, pattern)))
					continue;
				print_fds(info);
				break;

			case kFilterDevice:
				filter_device(info, device, brief);
				break;
			case kFilterFile:
				filter_file(info, device, node, brief);
				break;
		}
	}

	return 0;
}
