// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        listarea.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: lists area info for all currently running teams
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


#include <OS.h>

#include <stdio.h>
#include <stdlib.h>


void list_area_info(team_id id);
void show_memory_totals(void);


int
main(int argc, char **argv)
{
	show_memory_totals();

	if (argc == 1) {
		int32 cookie = 0;
		team_info info;

		// list for all teams
		while (get_next_team_info(&cookie, &info) >= B_OK)
			list_area_info(info.team);
	} else {
		// list for each team_id on the command line
		while (--argc)
			list_area_info(atoi(*++argv));
	}

	return 0;
}


void
show_memory_totals()
{
	int32 max = 0, used = 0;

	system_info info;
	if (get_system_info(&info) == B_OK) {
		// pages are 4KB
		max = info.max_pages * 4;
		used = info.used_pages * 4;
	}

	printf("memory: total: %4ldKB, used: %4ldKB, left: %4ldKB\n", max, used, max - used);
}


void
list_area_info(team_id id)
{	
	int32 cookie = 0;
	team_info teamInfo;
	area_info areaInfo;

	if (id != 1 && get_team_info(id, &teamInfo) == B_BAD_TEAM_ID) {
		printf("\nteam %ld unknown\n", id);
		return;
	} else if (id == 1)
		strcpy(teamInfo.args, "KERNEL SPACE");

	printf("\n%s (team %ld)\n", teamInfo.args, id);
	printf("  ID                              name   address     size   alloc. #-cow  #-in #-out\n");
	printf("------------------------------------------------------------------------------------\n");

	while (get_next_area_info(id, &cookie, &areaInfo) == B_OK) {
		printf("%5ld %32s  %08lx %8lx %8lx %5ld %5ld %5ld\n",
			areaInfo.area,
			areaInfo.name,
			(addr_t)areaInfo.address,
			areaInfo.size,
			areaInfo.ram_size,
			areaInfo.copy_count,
			areaInfo.in_count,
			areaInfo.out_count);
	}
}

