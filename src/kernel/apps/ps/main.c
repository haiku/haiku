/*
** Copyright 2002, Dan Sinclair. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * This code was originally in the shell/command.c file and
 * was written by:
 *  Damien Guard
 * I just split it out into its own file
*/

#include <syscalls.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define MAX_PROCESSES 1024

typedef struct proc_info proc_info;


inline const char *proc_state_string (int state);

inline
const char *
proc_state_string (int state)
{
	switch (state) {
		case 0: return "normal";
		case 1: return "birth";
		case 2: return "death";
		default:
			return "???";
	}
}


int
main(int argc, char ** argv)
{	
	size_t     table_size = MAX_PROCESSES * sizeof(proc_info);
	proc_info *table      = (proc_info *) malloc(table_size);

	if (table) {
		int num_procs = sys_proc_get_table(table, table_size);
	
		if (num_procs <= 0)
			printf("ps: sys_get_proc_table() returned error %s!\n", strerror(num_procs));
		
		else {
			int        n = num_procs;
			proc_info *p = table;
			
			printf("process list\n\n");
			printf("id\tstate\tthreads\tname\n");
			
			while (n--) {
				printf("%d\t%s\t%d\t%s\n",
					p->id,
					proc_state_string(p->state),
					p->num_threads,
					p->name);
				p += sizeof(proc_info);
			}
			printf("\n%d processes listed\n", num_procs);
		}

		free(table);
	}
	
	return 0;
}

