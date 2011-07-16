/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefere.de>
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <new>

#include <Entry.h>
#include <Errors.h>
#include <ObjectList.h>
#include <Path.h>

#include <package/RepositoryCache.h>
#include <package/RepositoryConfig.h>
#include <package/PackageInfo.h>
#include <package/PackageRoster.h>
#include <package/RepositoryInfo.h>

#include "pkgman.h"


// TODO: internationalization!


using namespace BPackageKit;


static const char* kCommandUsage =
	"Usage:\n"
	"    %s list-repos [options]\n"
	"Lists all configured package repositories.\n"
	"\n"
;


static void
print_command_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kCommandUsage, kProgramName);
    exit(error ? 1 : 0);
}


int
command_list_repos(int argc, const char* const* argv)
{
	bool verbose = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "verbose", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hv", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_command_usage_and_exit(false);
				break;

			case 'v':
				verbose = true;
				break;

			default:
				print_command_usage_and_exit(true);
				break;
		}
	}

	// No remaining arguments.
	if (argc != optind)
		print_command_usage_and_exit(true);

	BObjectList<BString> repositoryNames(20, true);
	BPackageRoster roster;
	status_t result = roster.GetRepositoryNames(repositoryNames);
	if (result != B_OK)
		DIE(result, "can't collect repository names");

	for (int i = 0; i < repositoryNames.CountItems(); ++i) {
		const BString& repoName = *(repositoryNames.ItemAt(i));
		BRepositoryConfig repoConfig;
		result = roster.GetRepositoryConfig(repoName, &repoConfig);
		if (result != B_OK) {
			BPath path;
			repoConfig.Entry().GetPath(&path);
			WARN(result, "skipping repository-config '%s'", path.Path());
			continue;
		}
		if (verbose && i > 0)
			printf("\n");
		printf(" %s %s\n",
			repoConfig.IsUserSpecific() ? "[User]" : "      ",
			repoConfig.Name().String());
		printf("\t\tbase-url:  %s\n", repoConfig.BaseURL().String());
		printf("\t\tpriority:  %u\n", repoConfig.Priority());

		if (verbose) {
			BRepositoryCache repoCache;
			result = roster.GetRepositoryCache(repoName, &repoCache);
			if (result == B_OK) {
				printf("\t\tvendor:    %s\n",
					repoCache.Info().Vendor().String());
				printf("\t\tsummary:   %s\n",
					repoCache.Info().Summary().String());
				printf("\t\tarch:      %s\n", BPackageInfo::kArchitectureNames[
						repoCache.Info().Architecture()]);
				printf("\t\tpkg-count: %lu\n", repoCache.CountPackages());
				printf("\t\torig-url:  %s\n",
					repoCache.Info().OriginalBaseURL().String());
				printf("\t\torig-prio: %u\n", repoCache.Info().Priority());

			} else
				printf("\t\t<no repository cache found>\n");
		}
	}

	return 0;
}
