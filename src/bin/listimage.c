/*
 * Copyright (c) 2001-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Daniel Reinhold, danielre@users.sf.net
 *		John Scipione, jscipione@gmail.com
 */

/*!	Lists image info for all currently running teams. */


#include <ctype.h>
#include <image.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>


static status_t
list_images_for_team_by_id(team_id id)
{
	image_info imageInfo;
	int32 cookie = 0;
	team_info teamInfo;
	char* header;
	char* format;
	int i;
	status_t result = get_team_info(id, &teamInfo);
	if (id != 1 && result < B_OK)
		return result;

	i = asprintf(&header, "   ID   %*s   %*s  Seq#      Init# Name",
		sizeof(uintptr_t) * 2, "Text", sizeof(uintptr_t) * 2, "Data");
	if (i == -1)
		return B_NO_MEMORY;

	i = asprintf(&format, "%%5" B_PRId32 " 0x%%0%" B_PRIu32 PRIxPTR
		" 0x%%0%" B_PRIu32 PRIxPTR "  %%4" B_PRId32 " %%10" B_PRIu32 " %%s\n",
		sizeof(uintptr_t) * 2, sizeof(uintptr_t) * 2);
	if (i == -1) {
		free(header);
		return B_NO_MEMORY;
	}

	if (id == 1)
		printf("\nKERNEL TEAM:\n");
	else
		printf("\nTEAM %4" B_PRId32 " (%s):\n", id, teamInfo.args);

	puts(header);
	for (i = 0; i < 80; i++)
		putchar('-');

	printf("\n");
	while ((result = get_next_image_info(id, &cookie, &imageInfo)) == B_OK) {
		printf(format, imageInfo.id, imageInfo.text, imageInfo.data,
			imageInfo.sequence, imageInfo.init_order, imageInfo.name);
	}

	free(header);
	free(format);

	if (result != B_ENTRY_NOT_FOUND && result != EINVAL) {
		printf("get images failed: %s\n", strerror(result));
		return result;
	}

	return B_OK;
}


static void
list_images_for_team(const char* arg)
{
	int32 cookie = 0;
	team_info info;
	status_t result;

	if (atoi(arg) > 0 && list_images_for_team_by_id(atoi(arg)) == B_OK)
		return;

	/* search for the team by name */

	while (get_next_team_info(&cookie, &info) >= B_OK) {
		if (strstr(info.args, arg)) {
			result = list_images_for_team_by_id(info.team);
			if (result != B_OK) {
				printf("\nCould not retrieve information about team %"
					B_PRId32 ": %s\n", info.team, strerror(result));
			}
		}
	}
}


int
main(int argc, char** argv)
{
	int32 cookie = 0;
	team_info info;
	status_t result;

	if (argc == 1) {
		/* list for all teams */
		while (get_next_team_info(&cookie, &info) >= B_OK) {
			result = list_images_for_team_by_id(info.team);
			if (result != B_OK) {
				printf("\nCould not retrieve information about team %"
					B_PRId32 ": %s\n", info.team, strerror(result));
			}
		}
	} else {
		/* list for each team_id on the command line */
		while (--argc > 0 && ++argv != NULL)
			list_images_for_team(*argv);
	}

	return 0;
}
