// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2002, OpenBeOS
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
#include <image.h>
#include <OS.h>


void list_image_info  (team_id id);



int
main (int argc, char **argv)
	{
	//	
	if (argc == 1)
		{
		// list for all teams
		int32     cookie = 0;
		team_info info;
		
		while (get_next_team_info (&cookie, &info) >= B_OK)
			{
			list_image_info (info.team);
			}
		}
	else
		// list for each team_id on the command line
		while (--argc)
			{
			list_image_info (atoi (*++argv));
			}
	
	return 0;
	}


void
list_image_info (team_id id)
	{	
	int32       cookie = 0;
	team_info   this_team;
	image_info  this_image;
	
	if (get_team_info (id, &this_team) == B_BAD_TEAM_ID)
		{
		printf ("\nteam %d unknown\n", id);
		return;
		}

	printf ("\nTEAM %4d (%s):\n", id, this_team.args);
	printf ("   ID                                                             name     text     data seq#      init#\n");
	printf ("--------------------------------------------------------------------------------------------------------\n");
	
	while (get_next_image_info (id, &cookie, &this_image) == B_OK)
		{
		printf ("%5d %64s %.8x %.8x %4d %10u\n",
		         this_image.id,
		         this_image.name,
		         this_image.text,
		         this_image.data,
		         this_image.sequence,
		         this_image.init_order);
		}
	}

