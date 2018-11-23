/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefere.de>
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <Errors.h>
#include <SupportDefs.h>

#include <package/DropRepositoryRequest.h>
#include <package/Context.h>

#include "Command.h"
#include "DecisionProvider.h"
#include "JobStateListener.h"
#include "pkgman.h"


using namespace BPackageKit;


// TODO: internationalization!


static const char* const kShortUsage =
	"  %command% <repo-name>\n"
	"    Drops the repository with the given <repo-name>.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% <repo-name>\n"
	"Drops (i.e. removes) the repository with the given name.\n"
	"\n";


DEFINE_COMMAND(DropRepoCommand, "drop-repo", kShortUsage, kLongUsage,
	kCommandCategoryRepositories)


int
DropRepoCommand::Execute(int argc, const char* const* argv)
{
	bool interactive = true;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "yes", no_argument, 0, 'y' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hu", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			case 'y':
				interactive = false;
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining argument is a repo name, i. e. one more argument.
	if (argc != optind + 1)
		PrintUsageAndExit(true);

	const char* repoName = argv[optind];

	DecisionProvider decisionProvider;
	decisionProvider.SetInteractive(interactive);
	JobStateListener listener;
	BContext context(decisionProvider, listener);

	status_t result;
	DropRepositoryRequest dropRequest(context, repoName);
	result = dropRequest.Process(true);
	if (result != B_OK) {
		if (result != B_CANCELED) {
			DIE(result, "request for dropping repository \"%s\" failed",
				repoName);
		}
		return 1;
	}

	return 0;
}
