/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <ctype.h>
#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "commands.h"
#include "parse.h"
#include "statements.h"
#include "shell_defs.h"
#include "shell_vars.h"
#include "args.h"
#include "shell_history.h"


#define ECHO 0
	// echoes input when unequal 0


static int
readline(char *buf, int len)
{	
	int  i = 0;
	char ch;

	while (true) {
		ch = getchar();
		
		switch (ch) {
		case 27:
			// ESC -- used to display command history:
			//   erase all characters on the current line,
			//   then display the previously saved command
#if ECHO
			while (i--) {
				printf("\b");
			}
#endif
			
			strcpy (buf, fetch_history_command());
			i = printf("%s", buf);
			break;
		
		case '\b':
			// backspace:
			//   erase the last character written
			if (i > 0) {
#if ECHO
				printf("\b");
#endif
				--i;
			}
			break;

		case '\n':
			buf[i] = 0;
			store_history_command(buf);
#if ECHO
			printf("\n");
#endif
			return i;

		default:
			buf[i] = ch;
#if ECHO
			printf("%c", ch);
#endif
			i++;
		}
	}
}


int
main(int argc, char *argv[])
{
	char buf[1024];

	init_vars();
	init_statements();
	init_arguments(argc, argv);

	if (af_script_file_name) {
		run_script(af_script_file_name);
		if (af_exit_after_script)
			exit(0);
	}

	if (gCommandToExecute) {
		if (strlen(gCommandToExecute) > 0)
			parse_string(gCommandToExecute);
		exit(0);
	}

	printf("Welcome to the Haiku shell\n");

	for (;;) {
		int chars_read;

		printf("$ ");

		chars_read = readline(buf, sizeof(buf));
 		if (chars_read > 0)
			parse_string(buf);
	}

	printf("shell exiting\n");
	return 0;
}


