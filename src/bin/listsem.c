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

	Copyright (c) 2002 Oliver Siebenmarck <olli@ithome.de>, Haiku project

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
#include <string.h>
#include <OS.h>


static void print_sem_info(sem_info *info)
{
	printf("%7" B_PRId32 "%31s%7" B_PRId32 "\n", info->sem, info->name,
		info->count);
}


static void print_header(team_info *tinfo)
{
	if (tinfo != NULL)
		printf("TEAM  %" B_PRId32 " (%s):\n", tinfo->team, tinfo->args);

	printf("     ID                           name  count\n");
	printf("---------------------------------------------\n");
}


static void list_sems(team_info *tinfo)
{
	sem_info info;
	int32 cookie = 0;

	print_header(tinfo);

	while (get_next_sem_info(tinfo->team, &cookie, &info) == B_OK)
		print_sem_info(&info);

	printf("\n");
}

int main(int argc, char **argv)
{
	team_info tinfo;
	int32 cookie = 0;
	int32 i;
	system_info sysinfo;

	// show up some stats first...
	get_system_info(&sysinfo);
	printf("sem: total: %5" B_PRIu32 ", used: %5" B_PRIu32 ", left: %5" B_PRIu32
		"\n\n", sysinfo.max_sems, sysinfo.used_sems,
		sysinfo.max_sems - sysinfo.used_sems);

	if (argc == 1) {
		while (get_next_team_info( &cookie, &tinfo) == B_OK)
			list_sems(&tinfo);

		return 0;
	}

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			fprintf(stderr, "Usage:  %s [-s semid]  [teamid]\n", argv[0]);
			fputs("        List the semaphores allocated by the specified\n",
				stderr);
			fputs("        team, or all teams if none is specified.\n",
				stderr);
			fputs("\n", stderr);
			fputs("        The -s option displays the sem_info data for a\n",
				stderr);
			fputs("        specified semaphore.\n", stderr);
			return 0;
		} else if (strcmp(argv[i], "-s") == 0) {
			if (argc < i + 2)
				printf("-s used without associated sem id\n");
			else {
				sem_id id = atoi(argv[i + 1]);
				sem_info info;
				if (get_sem_info(id, &info) == B_OK) {
					print_header(NULL);
					print_sem_info(&info);
				} else
					printf("semaphore %" B_PRId32 " unknown\n\n", id);

				i++;
			}
		} else {
			team_id team;
			team = atoi(argv[i]);
			if (get_team_info(team, &tinfo) == B_OK)
				list_sems(&tinfo);
			else
				printf("team %" B_PRId32 " unknown\n\n", team);
		}
	}

	return 0;
}

