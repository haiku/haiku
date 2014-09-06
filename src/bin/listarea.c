/*
 * Copyright (c) 2001-2005, Haiku.
 *
 * This software is part of the Haiku distribution and is covered 
 * by the MIT license.
 *
 * Author: Daniel Reinhold (danielre@users.sf.net)
 */

/** Lists area info for all currently running teams */


#include <OS.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void list_areas_for_id(team_id team);
static void list_areas_for_name(const char *teamName);
static void show_memory_totals(void);


static void
show_memory_totals(void)
{
	int32 max = 0, used = 0;

	system_info info;
	if (get_system_info(&info) == B_OK) {
		// pages are 4KB
		max = info.max_pages * 4;
		used = info.used_pages * 4;
	}

	printf("memory: total: %4" B_PRId32 "KB, used: %4" B_PRId32 "KB, left: %4"
		B_PRId32 "KB\n", max, used, max - used);
}


static void
list_areas_for_id(team_id id)
{	
	ssize_t cookie = 0;
	team_info teamInfo;
	area_info areaInfo;

	if (id != 1 && get_team_info(id, &teamInfo) == B_BAD_TEAM_ID) {
		printf("\nteam %" B_PRId32 " unknown\n", id);
		return;
	} else if (id == 1)
		strcpy(teamInfo.args, "KERNEL SPACE");

	printf("\n%s (team %" B_PRId32 ")\n", teamInfo.args, id);
	printf("   ID                             name   address     size   alloc."
		" #-cow  #-in #-out\n");
	printf("------------------------------------------------------------------"
		"------------------\n");

	while (get_next_area_info(id, &cookie, &areaInfo) == B_OK) {
		printf("%5" B_PRId32 " %32s  %p %8" B_PRIxSIZE " %8" B_PRIx32 " %5"
			B_PRId32 " %5" B_PRId32 " %5" B_PRId32 "\n",
			areaInfo.area,
			areaInfo.name,
			areaInfo.address,
			areaInfo.size,
			areaInfo.ram_size,
			areaInfo.copy_count,
			areaInfo.in_count,
			areaInfo.out_count);
	}
}


static void
list_areas_for_name(const char *name)
{
	int32 cookie = 0;
	team_info info;
	while (get_next_team_info(&cookie, &info) >= B_OK) {
		if (strstr(info.args, name) != NULL)
			list_areas_for_id(info.team);
	}
}


int
main(int argc, char **argv)
{
	show_memory_totals();

	if (argc == 1) {
		// list areas of all teams
		int32 cookie = 0;
		team_info info;

		while (get_next_team_info(&cookie, &info) >= B_OK)
			list_areas_for_id(info.team);
	} else {
		// list areas for each team ID/name on the command line
		while (--argc) {
			const char *arg = *++argv;
			team_id team = atoi(arg);

			// if atoi returns >0 it's likely it's a number, else take it as string
			if (team > 0)
				list_areas_for_id(team);
			else
				list_areas_for_name(arg);
		}
	}

	return 0;
}

