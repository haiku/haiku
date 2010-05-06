/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Arguments.h"

#include <Catalog.h>
#include <Locale.h>

Arguments::Arguments(int defaultArgsNum, const char * const *defaultArgs)
	: fUsageRequested(false),
	  fBounds(50, 50, 630, 435),
	  fStandardShell(true),
	  fFullScreen(false),
	  fShellArgumentCount(0),
	  fShellArguments(NULL),
	  fTitle(NULL)
{
	_SetShellArguments(defaultArgsNum, defaultArgs);
}


Arguments::~Arguments()
{
	_SetShellArguments(0, NULL);
}

#undef TR_CONTEXT
#define TR_CONTEXT "Terminal arguments parsing"

void
Arguments::Parse(int argc, const char *const *argv)
{
	int argi = 1;
	while (argi < argc) {
		const char *arg = argv[argi++];

		if (*arg == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				fUsageRequested = true;
			
			/*} else if (strcmp(arg, "-l") == 0) {
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
*/
			} else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--title") == 0) {
				// title
				if (argi >= argc)
					fUsageRequested = true;
				else
					fTitle = argv[argi++];

			} else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--fullscreen") == 0) {
				fFullScreen = true;				
				argi++;			
			} else {
				// illegal option
				fprintf(stderr, TR("Unrecognized option \"%s\"\n"), arg);
				fUsageRequested = true;
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
	for (int32 i = 0; i < fShellArgumentCount; i++)
		free((void *)fShellArguments[i]);
	delete[] fShellArguments;

	fShellArguments = NULL;
	fShellArgumentCount = 0;

	// copy new ones
	if (argc > 0 && argv) {
		fShellArguments = new const char*[argc + 1];
		for (int i = 0; i < argc; i++)
			fShellArguments[i] = strdup(argv[i]);

		fShellArguments[argc] = NULL;
		fShellArgumentCount = argc;
	}
}

