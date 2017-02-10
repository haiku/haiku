// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2004, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered
//  by the MIT License.
//
//
//  File:        release.c
//  Author:      Jérôme Duval (korli@chez.com)
//  Description: release a semaphore.
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>

static int usage(char *prog)
{
	printf("usage: release [-f] [-c count] semaphore_number\n");
	printf("warning: this can be hazardous...\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int count = 1;
	bool force = false;
	sem_id semid = -1;
	status_t err;
	team_id teamid = -1;
	
	int i;
	if (argc < 2 || argc > 5)
		usage("release");
	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-f", 2) == 0) {
			force = true;
		} else if (strncmp(argv[i], "-c", 2) == 0) {
			i++;
			if (i < argc) {
				count = strtol(argv[i], NULL, 10);
			} else
				usage("release");
		} else {
			semid = strtol(argv[i], NULL, 10);
		}
	}
	
	if (semid <=0)
		usage("release");
	
	if (force) {
		sem_info sinfo;
		thread_info tinfo;
		get_sem_info(semid, &sinfo);
		teamid = sinfo.team;
		get_thread_info(find_thread(NULL), &tinfo);
		set_sem_owner(semid, tinfo.team);
	}
	
	printf("releasing semaphore %d\n", (int)semid);
	err = release_sem_etc(semid, count, 0);
		
	if (err < B_OK) {
		fprintf(stderr, "release_sem failed: %s\n", strerror(err));
		return 1;
	} else if (force) {
		set_sem_owner(semid, teamid);
	}

	return 0;	
}
