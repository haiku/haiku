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

#include "Command.h"
#include "DecisionProvider.h"
#include "JobStateListener.h"
#include "pkgman.h"


using namespace BPackageKit;


// TODO: internationalization!


static const char* const kShortUsage =
	"  %command% [<repo-name> ...]\n"
	"    Refreshes all or just the given repositories.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% [<repo-name> ...]\n"
	"Refreshes all or just the given repositories.\n"
	"\n";


DEFINE_COMMAND(RefreshCommand, "refresh", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_REPOSITORIES)


int
RefreshCommand::Execute(int argc, const char* const* argv)
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
				PrintUsageAndExit(false);
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining arguments are repo names.
	const char* const* repoArgs = argv + optind;
	int nameCount = argc - optind;

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

	DecisionProvider decisionProvider;
	JobStateListener listener;
	BContext context(decisionProvider, listener);

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
		result = refreshRequest.Process();
		if (result != B_OK) {
			DIE(result, "request for refreshing repository \"%s\" failed",
				repoName.String());
		}
	}

	return 0;
}
