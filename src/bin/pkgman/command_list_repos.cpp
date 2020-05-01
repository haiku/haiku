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

#include "Command.h"
#include "pkgman.h"


// TODO: internationalization!


using namespace BPackageKit;


static const char* const kShortUsage =
	"  %command%\n"
	"    Lists all repositories.\n";

static const char* const kLongUsage =
	"Usage:\n"
	"    %program% %command% [options]\n"
	"Lists all configured package repositories.\n"
	"\n";


DEFINE_COMMAND(ListReposCommand, "list-repos", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_REPOSITORIES)


int
ListReposCommand::Execute(int argc, const char* const* argv)
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
				PrintUsageAndExit(false);
				break;

			case 'v':
				verbose = true;
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// No remaining arguments.
	if (argc != optind)
		PrintUsageAndExit(true);

	BStringList repositoryNames(20);
	BPackageRoster roster;
	status_t result = roster.GetRepositoryNames(repositoryNames);
	if (result != B_OK)
		DIE(result, "can't collect repository names");

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
		if (verbose && i > 0)
			printf("\n");
		printf(" %s %s\n",
			repoConfig.IsUserSpecific() ? "[User]" : "      ",
			repoConfig.Name().String());
		printf("\t\tbase-url:  %s\n", repoConfig.BaseURL().String());
		printf("\t\tidentifier: %s\n", repoConfig.Identifier().String());
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
				printf("\t\tpkg-count: %" B_PRIu32 "\n",
					repoCache.CountPackages());
				printf("\t\tbase-url:  %s\n",
					repoCache.Info().BaseURL().String());
				printf("\t\tidentifier:  %s\n",
					repoCache.Info().Identifier().String());
				printf("\t\torig-prio: %u\n", repoCache.Info().Priority());
			} else
				printf("\t\t<no repository cache found>\n");
		}
	}

	return 0;
}
