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

#include "DecisionProvider.h"
#include "JobStateListener.h"
#include "pkgman.h"


using namespace BPackageKit;


// TODO: internationalization!


static const char* kCommandUsage =
	"Usage: %s drop-repo <repo-name>\n"
	"Drops (i.e. removes) the repository with the given name.\n"
	"\n"
;


static void
print_command_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kCommandUsage, kProgramName);
    exit(error ? 1 : 0);
}


int
command_drop_repo(int argc, const char* const* argv)
{
	bool yesMode = false;

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
				print_command_usage_and_exit(false);
				break;

			case 'y':
				yesMode = true;
				break;

			default:
				print_command_usage_and_exit(true);
				break;
		}
	}

	// The remaining argument is a repo name, i. e. one more argument.
	if (argc != optind + 1)
		print_command_usage_and_exit(true);

	const char* repoName = argv[optind];

	DecisionProvider decisionProvider;
//	if (yesMode)
//		decisionProvider.SetAcceptEverything(true);
	JobStateListener listener;
	BContext context(decisionProvider, listener);

	status_t result;
	DropRepositoryRequest dropRequest(context, repoName);
	result = dropRequest.InitCheck();
	if (result != B_OK)
		DIE(result, "unable to create request for dropping repository");
	result = dropRequest.CreateInitialJobs();
	if (result != B_OK)
		DIE(result, "unable to create necessary jobs");

	while (BJob* job = dropRequest.PopRunnableJob()) {
		result = job->Run();
		delete job;
		if (result == B_CANCELED)
			return 1;
	}

	return 0;
}
