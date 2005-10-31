/*
 * Copyright (c) 2001-2005, Haiku.
 *
 * This software is part of the Haiku distribution and is covered 
 * by the MIT license.
 *
 * Author: Daniel Reinhold (danielre@users.sf.net)
 */

/**	Description: lists image info for all currently running teams */


#include <OS.h>
#include <image.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


static status_t
list_images_for_team_by_id(team_id id)
{
	team_info teamInfo;
	image_info imageInfo;
	int32 cookie = 0;
	status_t status;

	status = get_team_info(id, &teamInfo);
	if (id != 1 && status < B_OK)
		return status;

	if (id == 1)
		printf("\nKERNEL TEAM:\n");
	else
		printf("\nTEAM %4ld (%s):\n", id, teamInfo.args);
	printf("   ID                                                             name       text       data seq#      init#\n");
	printf("------------------------------------------------------------------------------------------------------------\n");

	while ((status = get_next_image_info(id, &cookie, &imageInfo)) == B_OK) {
		printf("%5ld %64s %p %p %4ld %10lu\n",
			imageInfo.id,
			imageInfo.name,
			imageInfo.text,
			imageInfo.data,
			imageInfo.sequence,
			imageInfo.init_order);
	}
	if (status != B_ENTRY_NOT_FOUND && status != EINVAL) {
		printf("get images failed: %s\n", strerror(status));
		return status;
	}
	return B_OK;
}


static void
list_images_for_team(const char *arg)
{
	int32 cookie = 0;
	team_info info;

	if (atoi(arg) > 0) {
		if (list_images_for_team_by_id(atoi(arg)) == B_OK)
			return;
	}

	// search for the team by name

	while (get_next_team_info(&cookie, &info) >= B_OK) {
		if (strstr(info.args, arg)) {
			status_t status = list_images_for_team_by_id(info.team);
			if (status != B_OK)
				printf("\nCould not retrieve information about team %ld: %s\n",
					info.team, strerror(status));
		}
	}
}


int
main(int argc, char **argv)
{
	if (argc == 1) {
		int32 cookie = 0;
		team_info info;

		// list for all teams
		while (get_next_team_info(&cookie, &info) >= B_OK) {
			status_t status = list_images_for_team_by_id(info.team);
			if (status != B_OK)
				printf("\nCould not retrieve information about team %ld: %s\n",
					info.team, strerror(status));
		}
	} else {
		// list for each team_id on the command line
		while (--argc)
			list_images_for_team(*++argv);
	}
	return 0;
}

