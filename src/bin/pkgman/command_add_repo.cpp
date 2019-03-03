/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefere.de>
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <Errors.h>
#include <SupportDefs.h>
#include <Url.h>

#include <package/AddRepositoryRequest.h>
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
	"  %command% <repo-base-url>\n"
	"    Adds the repository with the given <repo-base-URL>.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% <repo-URL> [<repo-URL> ...]\n"
	"Adds one or more repositories by downloading them from the given URL(s).\n"
	"\n";


DEFINE_COMMAND(AddRepoCommand, "add-repo", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_REPOSITORIES)


int
AddRepoCommand::Execute(int argc, const char* const* argv)
{
	bool asUserRepository = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "user", no_argument, 0, 'u' },
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

			case 'u':
				asUserRepository = true;
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining arguments are repo URLs, i. e. at least one more argument.
	if (argc < optind + 1)
		PrintUsageAndExit(true);

	const char* const* repoURLs = argv + optind;
	int urlCount = argc - optind;

	DecisionProvider decisionProvider;
	JobStateListener listener;
	BContext context(decisionProvider, listener);

	status_t result;
	for (int i = 0; i < urlCount; ++i) {
		// Test if a valid URL has been supplied before attempting to add
		BUrl repoURL(repoURLs[i]);
		if (!repoURL.IsValid()) {
			result = B_BAD_VALUE;
			DIE(result, "request for adding repository \"%s\" failed",
				repoURLs[i]);
		}
		AddRepositoryRequest addRequest(context, repoURLs[i], asUserRepository);
		result = addRequest.Process(true);
		if (result != B_OK) {
			if (result != B_CANCELED) {
				DIE(result, "request for adding repository \"%s\" failed",
					repoURLs[i]);
			}
			return 1;
		}

		// now refresh the repo-cache of the new repository
		BString repoName = addRequest.RepositoryName();
		BPackageRoster roster;
		BRepositoryConfig repoConfig;
		roster.GetRepositoryConfig(repoName, &repoConfig);

		BRefreshRepositoryRequest refreshRequest(context, repoConfig);
		result = refreshRequest.Process(true);
		if (result != B_OK) {
			if (result != B_CANCELED) {
				DIE(result, "request for refreshing repository \"%s\" failed",
					repoName.String());
			}
			return 1;
		}
	}

	return 0;
}
