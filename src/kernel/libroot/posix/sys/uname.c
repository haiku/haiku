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

	strcpy(info->sysname, "Haiku");
	strlcpy(info->version, sinfo.kernel_build_date, B_OS_NAME_LENGTH);
	strlcat(info->version, sinfo.kernel_build_time, B_OS_NAME_LENGTH);
	sprintf(info->release, "%lld", sinfo.kernel_version);

	// TODO fill nodename field when we have hostname info
	strcpy(info->nodename, "unknown");

	return 0;
}

