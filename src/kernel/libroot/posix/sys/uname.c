/*
** Copyright 2004, Jérôme Duval jerome.duval@free.fr. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <sys/utsname.h>
#include <OS.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int
uname(struct utsname *info)
{
	system_info sinfo;

	if (!info) {
		errno = B_BAD_VALUE;
		return -1;
	}

	get_system_info(&sinfo);

	strlcpy(info->sysname, "Haiku", sizeof(info->sysname));
	strlcpy(info->version, sinfo.kernel_build_date, sizeof(info->version));
	strlcat(info->version, sinfo.kernel_build_time, sizeof(info->version));
	snprintf(info->release, sizeof(info->release), "%lld", sinfo.kernel_version);

	// TODO fill machine field...
	strlcpy(info->machine, "unknown", sizeof(info->machine));
	
	// TODO fill nodename field when we have hostname info
	strlcpy(info->nodename, "unknown", sizeof(info->nodename));

	return 0;
}

