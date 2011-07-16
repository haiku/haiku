/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefere.de>
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <Errors.h>
#include <StringList.h>

#include <package/Context.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/PackageRoster.h>

#include "DecisionProvider.h"
#include "JobStateListener.h"
#include "pkgman.h"


using namespace BPackageKit;


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

	DecisionProvider decisionProvider;
	JobStateListener listener;
	BContext context(decisionProvider, listener);

	BStringList repositoryNames(20);

	BPackageRoster roster;
	if (nameCount == 0) {
		status_t result = roster.GetRepositoryNames(repositoryNames);
		if (result != B_OK)
			DIE(result, "can't collect repository names");
	} else {
		for (int i = 0; i < nameCount; ++i) {
			if (!repositoryNames.Add(repoArgs[i]))
				DIE(B_NO_MEMORY, "can't allocate repository name");
		}
	}

	status_t result;
	for (int i = 0; i < repositoryNames.CountStrings(); ++i) {
		const BString& repoName = repositoryNames.StringAt(i);
		BRepositoryConfig repoConfig;
		result = roster.GetRepositoryConfig(repoName, &repoConfig);
		if (result != B_OK) {
			BPath path;
			repoConfig.Entry().GetPath(&path);
			WARN(result, "skipping repository-config '%s'", path.Path());
			continue;
		}
		BRefreshRepositoryRequest refreshRequest(context, repoConfig);
		result = refreshRequest.InitCheck();
		if (result != B_OK)
			DIE(result, "unable to create request for refreshing repository");
		result = refreshRequest.CreateInitialJobs();
		if (result != B_OK)
			DIE(result, "unable to create necessary jobs");

		while (BJob* job = refreshRequest.PopRunnableJob()) {
			result = job->Run();
			delete job;
			if (result != B_OK)
				return 1;
		}
	}

	return 0;
}
