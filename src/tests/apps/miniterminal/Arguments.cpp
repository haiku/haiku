/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Arguments.h"

extern const char *__progname;

// usage
static const char *kUsage =
	"%s <options> [ <program> ... ]\n"
	"Starts a terminal with a shell running in it. If <program> is given\n"
	"and is an absolute or relative path to an executable, it is run instead\n"
	"of the shell. The command line arguments for the program just follow the\n"
	"path of the program.\n"
	"\n"
	"Options:\n"
	"  -h, --help           - print this info text\n"
	"  -l <x> <y>           - open the terminal window at location (<x>, <y>)\n"
	"  -s <width> <height>  - open the terminal window with width <width> and\n"
	"                         height <height>)\n"
	"  -t <title>           - set the terminal window title to <title>\n";

// application name
const char *kAppName = __progname;

static void
print_usage(bool error)
{
	fprintf(error ? stderr : stdout, kUsage, kAppName);
}

static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 0 : 1);
}

Arguments::Arguments()
	: fBounds(50, 50, 630, 435),
	  fStandardShell(true),
	  fShellArgumentCount(0),
	  fShellArguments(NULL),
	  fTitle("MiniTerminal")
{
	const char *argv[] = { "/bin/sh", "--login" };

	_SetShellArguments(2, argv);
}


Arguments::~Arguments()
{
	_SetShellArguments(0, NULL);
}


void
Arguments::Parse(int argc, const char *const *argv)
{
	int argi = 1;
	while (argi < argc) {
		const char *arg = argv[argi++];

		if (*arg == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);

			} else if (strcmp(arg, "-l") == 0) {
				// location
				float x, y;
				if (argi + 1 >= argc
					|| sscanf(argv[argi++], "%f", &x) != 1
					|| sscanf(argv[argi++], "%f", &y) != 1) {
					print_usage_and_exit(true);
				}

				fBounds.OffsetTo(x, y);

			} else if (strcmp(arg, "-s") == 0) {
				// size
				float width, height;
				if (argi + 1 >= argc
					|| sscanf(argv[argi++], "%f", &width) != 1
					|| sscanf(argv[argi++], "%f", &height) != 1) {
					print_usage_and_exit(true);
				}

				fBounds.right = fBounds.left + width;
				fBounds.bottom = fBounds.top + height;

			} else if (strcmp(arg, "-t") == 0) {
				// title
				if (argi >= argc)
					print_usage_and_exit(true);

				fTitle = argv[argi++];

			} else {
				// illegal option
				fprintf(stderr, "Unrecognized option \"%s\"\n", arg);
				print_usage_and_exit(true);
			}

		} else {
			// no option, so the remainder is the shell program with arguments
			_SetShellArguments(argc - argi + 1, argv + argi - 1);
			argi = argc;
			fStandardShell = false;
		}
	}
}


void
Arguments::GetShellArguments(int &argc, const char *const *&argv) const
{
	argc = fShellArgumentCount;
	argv = fShellArguments;
}


void
Arguments::_SetShellArguments(int argc, const char *const *argv)
{
	// delete old arguments
	delete[] fShellArguments;
	fShellArguments = NULL;
	fShellArgumentCount = 0;

	// copy new ones
	if (argc > 0 && argv) {
		fShellArguments = new const char*[argc + 1];
		for (int i = 0; i < argc; i++)
			fShellArguments[i] = argv[i];

		fShellArguments[argc] = NULL;
		fShellArgumentCount = argc;
	}
}

