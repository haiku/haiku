/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>


char *
getpass(const char *prompt)
{
	static char password[128];
	struct termios termios;
	bool changed = false;

	// Turn off echo

	if (tcgetattr(fileno(stdin), &termios) == 0) {
		struct termios noEchoTermios = termios;

		noEchoTermios.c_lflag &= ~(ECHO | ISIG);
		changed = tcsetattr(fileno(stdin), TCSAFLUSH, &noEchoTermios) == 0;
    }

	// Show prompt
	fputs(prompt, stdout);
	fflush(stdout);

	// Read password
	if (fgets(password, sizeof(password), stdin) != NULL) {
		size_t length = strlen(password);

		if (length > 0 && (password[length - 1] == '\n'))
			password[length - 1] = '\0';

		if (changed) {
			// Manually move to the next line
			putchar('\n');
		}
	}

	// Restore termios setting
	if (changed)
		tcsetattr(fileno(stdin), TCSAFLUSH, &termios);

	return password;
}
