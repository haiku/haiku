// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        listimage.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: lists image info for all currently running teams
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <image.h>
#include <OS.h>


void list_image_info (team_id id);


int
main(int argc, char **argv)
{
	if (argc == 1) {
		int32 cookie = 0;
		team_info info;

		// list for all teams
		while (get_next_team_info(&cookie, &info) >= B_OK)
			list_image_info(info.team);
	} else {
		// list for each team_id on the command line
		while (--argc)
			list_image_info(atoi(*++argv));
	}
	return 0;
}


void
list_image_info(team_id id)
{
	team_info teamInfo;
	image_info imageInfo;
	int32 cookie = 0;
	status_t status;

	status = get_team_info(id, &teamInfo);
	if (status < B_OK) {
		printf("\nCould not retrieve information about team %ld: %s\n", id, strerror(status));
		return;
	}

	printf("\nTEAM %4ld (%s):\n", id, teamInfo.args);
	printf("   ID                                                             name     text     data seq#      init#\n");
	printf("--------------------------------------------------------------------------------------------------------\n");

	while ((status = get_next_image_info(id, &cookie, &imageInfo)) == B_OK) {
		printf("%5ld %64s %p %p %4ld %10lu\n",
			imageInfo.id,
			imageInfo.name,
			imageInfo.text,
			imageInfo.data,
			imageInfo.sequence,
			imageInfo.init_order);
	}
	if (status != B_ENTRY_NOT_FOUND)
		printf("get images failed: %s\n", strerror(status));
}

