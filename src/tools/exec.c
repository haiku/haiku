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
#include <unistd.h>


static void
append_char(char c, char** arg, int* argLen, int* argBufferLen)
{
	if ((*argLen + 1) >= *argBufferLen) {
		*arg = realloc(*arg, *argBufferLen + 32);
		if (*arg == NULL) {
			puts("exec: oom");
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
			puts("exec: mismatched quotes");
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
		currentArgBufferLen = 0, encounteredNewlineAt = -1,
		modifiedEnvironment = 0, pos;

	if (argc != 2) {
		printf("usage: %s \"program arg 'arg1' ...\"\n", argv[0]);
		return 1;
	}

	str = argv[1];
	pos = 0;
	while (1) {
		switch (str[pos]) {
		case '\r':
		case '\n':
			// In normal shells, this would imply a second command.
			// We don't support that here, so we need to make sure
			// that either we have not parsed any arguments yet,
			// or there are no more arguments pushed after this.
			if (argsLen == 0 && currentArgLen == 0 && !modifiedEnvironment)
				break;
			encounteredNewlineAt = argsLen + 1;
			// fall through
		case ' ':
		case '\t':
		case '\0':
			if (currentArgLen == 0)
				break; // do nothing
			if (encounteredNewlineAt == argsLen) {
				puts("exec: running multiple commands not supported!");
				return 1;
			}

			// the current argument hasn't been terminated, do that now
			append_char('\0', &currentArg, &currentArgLen,
				&currentArgBufferLen);

			// handle environs
			{
			char* val;
			if (argsLen == 0 && (val = strstr(currentArg, "=")) != NULL) {
				const char* dollar;
				char* newVal = NULL;
				*val = '\0';
				val++;

				// handle trivial variable substitution, i.e. VAL=$VAL:...
				dollar = strstr(val, "$");
				if (dollar != NULL) {
					const char* oldVal;
					int oldValLen, valLen, nameLen;

					if (dollar != val) {
						puts("exec: environ expansion not at start of "
							"line unsupported");
						return 1;
					}
					val++; // skip the $
					valLen = strlen(val);
					nameLen = strlen(currentArg);

					// if the new value does not start with the environ name
					// (which is broken by a non-alphanumeric character), bail.
					if (strncmp(val, currentArg, nameLen) != 0
							|| isalnum(val[nameLen])) {
						puts("exec: environ expansion of other variables "
							"unsupported");
						return 1;
					}

					// get the old value and actually do the expansion
					oldVal = getenv(currentArg);
					oldValLen = strlen(oldVal);
					newVal = malloc(valLen + oldValLen + 1);
					memcpy(newVal, oldVal, oldValLen);
					memcpy(newVal + oldValLen, val + nameLen, valLen + 1);
					val = newVal;
				}

				setenv(currentArg, val, 1);
				free(newVal);
				modifiedEnvironment = 1;

				free(currentArg);
				currentArg = NULL;
				currentArgLen = 0;
				currentArgBufferLen = 0;
				break;
			}
			}

			// actually add the argument to the array
			if ((argsLen + 2) >= argsBufferLen) {
				args = realloc(args, (argsBufferLen + 8) * sizeof(char*));
				if (args == NULL) {
					puts("exec: oom");
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
			// don't append newlines to the current argument
			if (str[pos] == '\r' || str[pos] == '\n')
				break;
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
