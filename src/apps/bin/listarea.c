// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2002, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        listarea.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: lists area info for all currently running teams
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <stdlib.h>
#include <OS.h>


void list_area_info  (team_id id);
void show_memory_totals (void);



int
main (int argc, char **argv)
	{
	//
	show_memory_totals ();
	
	if (argc == 1)
		{
		// list for all teams
		int32     cookie = 0;
		team_info info;
		
		while (get_next_team_info (&cookie, &info) >= B_OK)
			{
			list_area_info (info.team);
			}
		}
	else
		// list for each team_id on the command line
		while (--argc)
			{
			list_area_info (atoi (*++argv));
			}
	
	return 0;
	}


void
show_memory_totals ()
	{
	int32       max = 0, used = 0, left;
	system_info sys;
	
	if (get_system_info (&sys) == B_OK)
		{
		// pages are 4KB
		max  = sys.max_pages * 4;
		used = sys.used_pages * 4;
		}
	
	left = max - used;
	
	printf ("memory: total: %4dKB, used: %4dKB, left: %4dKB\n", max, used, left);
	}


void
list_area_info (team_id id)
	{	
	int32      cookie = 0;
	team_info  this_team;
	area_info  this_area;
	
	if (get_team_info (id, &this_team) == B_BAD_TEAM_ID)
		{
		printf ("\nteam %d unknown\n", id);
		return;
		}

	printf ("\nTEAM %4d (%s):\n", id, this_team.args);
	printf ("  ID                           name  address     size   alloc. #-cow  #-in #-out\n");
	printf ("--------------------------------------------------------------------------------\n");
	
	while (get_next_area_info (id, &cookie, &this_area) == B_OK)
		{
		printf ("%5d %29s %.8x %8x %8x %5d %5d %5d\n",
		         this_area.area,
		         this_area.name,
		         this_area.address,
		         this_area.size,
		         this_area.ram_size,
		         this_area.copy_count,
		         this_area.in_count,
		         this_area.out_count);
		}
	}

