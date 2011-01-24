/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefere.de>
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <Errors.h>
#include <SupportDefs.h>

#include <package/Context.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/Roster.h>

#include "MyDecisionProvider.h"
#include "MyJobStateListener.h"
#include "pkgman.h"


using namespace Haiku::Package;


// TODO: internationalization!


static const char* kCommandUsage =
	"Usage: %s refresh [<repo-name> ...]\n"
	"Refreshes all or just the given repositories.\n"
	"\n"
;


static void
print_command_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kCommandUsage, kProgramName);
    exit(error ? 1 : 0);
}


int
command_refresh(int argc, const char* const* argv)
{
	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
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

			default:
				print_command_usage_and_exit(true);
				break;
		}
	}

	// The remaining arguments are repo names.
	const char* const* repoArgs = argv + optind;
	int nameCount = argc - optind;

	MyDecisionProvider decisionProvider;
	Context context(decisionProvider);
	MyJobStateListener listener;
	context.SetJobStateListener(&listener);

	BObjectList<BString> repositoryNames(20, true);

	Roster roster;
	if (nameCount == 0) {
		status_t result = roster.GetRepositoryNames(repositoryNames);
		if (result != B_OK)
			DIE(result, "can't collect repository names");
	} else {
		for (int i = 0; i < nameCount; ++i) {
			BString* repoName = new (std::nothrow) BString(repoArgs[i]);
			if (repoName == NULL)
				DIE(B_NO_MEMORY, "can't allocate repository name");
			repositoryNames.AddItem(repoName);
		}
	}

	status_t result;
	for (int i = 0; i < repositoryNames.CountItems(); ++i) {
		const BString& repoName = *(repositoryNames.ItemAt(i));
		RepositoryConfig repoConfig;
		result = roster.GetRepositoryConfig(repoName, &repoConfig);
		if (result != B_OK) {
			BPath path;
			repoConfig.Entry().GetPath(&path);
			WARN(result, "skipping repository-config '%s'", path.Path());
			continue;
		}
		RefreshRepositoryRequest refreshRequest(context, repoConfig);
		result = refreshRequest.CreateInitialJobs();
		if (result != B_OK)
			DIE(result, "unable to create necessary jobs");

		while (Job* job = refreshRequest.PopRunnableJob()) {
			result = job->Run();
			delete job;
			if (result != B_OK)
				return 1;
		}
	}

	return 0;
}
