/*
 *	listsem.c
 *
 *	Lists all semaphores in all Teams. 
 *	by O.Siebenmarck.
 * 	
 *      04-27-2002 - mmu_man
 *       added command line args
 *      
 *	Legal stuff follows:  
 
	Copyright (c) 2002 Oliver Siebenmarck <olli@ithome.de>, OpenBeOS project

	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in
	the Software without restriction, including without limitation the rights to
	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is furnished to do
	so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <OS.h>

static void list_sems(team_info *tinfo)
{
	sem_info info;
	int32 cookie = 0;

	printf("TEAM  %ld (%s):\n", tinfo->team, tinfo->args );
	printf("     ID                           name  count\n");
	printf("---------------------------------------------\n");
	
	while (get_next_sem_info(tinfo->team, &cookie, &info) == B_OK)
	{
		printf("%7ld%31s%7ld\n", info.sem ,info.name , info.count );
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	team_info tinfo;
	int32 cook = 0;
	int i;
	system_info sysinfo;

	// show up some stats first...
	get_system_info(&sysinfo);
	printf("sem: total: %5li, used: %5li, left: %5li\n\n", sysinfo.max_sems, sysinfo.used_sems, sysinfo.max_sems - sysinfo.used_sems);

	if (argc == 1) {
		while (get_next_team_info( &cook, &tinfo) == B_OK)
		{
			list_sems(&tinfo);
		}
		return 0;
	}
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			fprintf(stderr, "Usage:  %s [-s semid]  [teamid]\n", argv[0]);
			fputs("        List the semaphores allocated by the specified\n", stderr);
			fputs("        team, or all teams if none is specified.\n", stderr);
			fputs("\n", stderr);
			fputs("        The -s option displays the sem_info data for a\n", stderr);
			fputs("        specified semaphore.\n", stderr);
			return 0;
		} else {
			int t;
			t = atoi(argv[i]);
			if (get_team_info(t, &tinfo) == B_OK)
				list_sems(&tinfo);
			else
				printf("team %i unknown\n\n", t);
		}
	}
	return 0;
}

