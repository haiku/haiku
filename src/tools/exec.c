/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */

/* Pass this tool a string, and it will parse it into an argv and execvp(). */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


static void
append_char(char c, char** arg, int* argLen, int* argBufferLen)
{
	if ((*argLen + 1) >= *argBufferLen) {
		*arg = realloc(*arg, *argBufferLen + 32);
		if (*arg == NULL) {
			puts("oom");
			exit(1);
		}
		*argBufferLen += 32;
	}

	(*arg)[*argLen] = c;
	(*argLen)++;
}


static void
parse_quoted(const char* str, int* pos, char** currentArg, int* currentArgLen,
	int* currentArgBufferLen)
{
	char end = str[*pos];
	while (1) {
		char c;
		(*pos)++;
		c = str[*pos];
		if (c == '\0') {
			puts("mismatched quotes");
			exit(1);
		}
		if (c == end)
			break;

		switch (c) {
		case '\\':
			(*pos)++;
			// fall through
		default:
			append_char(str[*pos], currentArg, currentArgLen,
				currentArgBufferLen);
		break;
		}
	}
}


int
main(int argc, const char* argv[])
{
	char** args = NULL, *currentArg = NULL;
	const char* str;
	int argsLen = 0, argsBufferLen = 0, currentArgLen = 0,
		currentArgBufferLen = 0, pos;

	if (argc != 2) {
		printf("usage: %s \"program arg 'arg1' ...\"\n", argv[0]);
		return 1;
	}

	str = argv[1];
	pos = 0;
	while (1) {
		switch (str[pos]) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case '\0':
			if (currentArgLen == 0)
				break; // do nothing

			append_char('\0', &currentArg, &currentArgLen,
				&currentArgBufferLen);

			if ((argsLen + 2) >= argsBufferLen) {
				args = realloc(args, (argsBufferLen + 8) * sizeof(char*));
				if (args == NULL) {
					puts("oom");
					return 1;
				}
				argsBufferLen += 8;
			}

			args[argsLen] = currentArg;
			args[argsLen + 1] = NULL;
			argsLen++;

			currentArg = NULL;
			currentArgLen = 0;
			currentArgBufferLen = 0;
		break;

		case '\'':
		case '"':
			parse_quoted(str, &pos, &currentArg, &currentArgLen,
				&currentArgBufferLen);
		break;

		case '\\':
			pos++;
			// fall through
		default:
			append_char(str[pos], &currentArg, &currentArgLen,
				&currentArgBufferLen);
		break;
		}
		if (str[pos] == '\0')
			break;
		pos++;
	}

	pos = execvp(args[0], args);
	if (pos != 0)
		printf("exec failed: %s\n", strerror(errno));
	return pos;
}
