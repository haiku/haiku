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

#include <package/RepositoryConfig.h>
#include <package/Roster.h>

#include "pkgman.h"


// TODO: internationalization!


using namespace Haiku::Package;


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


typedef BObjectList<RepositoryConfig> RepositoryConfigList;


struct RepositoryConfigCollector : public RepositoryConfigVisitor {
	RepositoryConfigCollector(RepositoryConfigList& _repositoryConfigList);

	status_t operator()(const BEntry& entry);

	RepositoryConfigList& repositoryConfigList;
};


RepositoryConfigCollector::RepositoryConfigCollector(
	RepositoryConfigList& _repositoryConfigList)
	:
	repositoryConfigList(_repositoryConfigList)
{
}


status_t
RepositoryConfigCollector::operator()(const BEntry& entry)
{
	RepositoryConfig* repoConfig = new (std::nothrow) RepositoryConfig(entry);
	if (repoConfig == NULL)
		DIE(B_NO_MEMORY, "can't create repository-config object");

	status_t result = repoConfig->InitCheck();
	if (result != B_OK) {
		BPath path;
		entry.GetPath(&path);
		WARN(result, "skipping repository-config '%s'", path.Path());
		delete repoConfig;

		return B_OK;
			// let collector continue
	}

	return repositoryConfigList.AddItem(repoConfig) ? B_OK : B_NO_MEMORY;
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

	Roster roster;
	RepositoryConfigList repositoryConfigs;
	RepositoryConfigCollector repositoryConfigCollector(repositoryConfigs);
	status_t result
		= roster.VisitCommonRepositoryConfigs(repositoryConfigCollector);
	if (result != B_OK && result != B_ENTRY_NOT_FOUND)
		DIE(result, "can't collect common repository configs");

	result = roster.VisitUserRepositoryConfigs(repositoryConfigCollector);
	if (result != B_OK && result != B_ENTRY_NOT_FOUND)
		DIE(result, "can't collect user's repository configs");

	int32 count = repositoryConfigs.CountItems();
	for (int32 i = 0; i < count; ++i) {
		RepositoryConfig* repoConfig = repositoryConfigs.ItemAt(i);
		printf("    %s %s\n",
			repoConfig->IsUserSpecific() ? "[User]" : "      ",
			repoConfig->Name().String());
	}

	return 0;
}
