/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <package/PackageRoster.h>
#include <package/RepositoryConfig.h>

#include <AutoDeleter.h>

#include "pkgman.h"
#include "RepositoryBuilder.h"


// TODO: internationalization!
// The printing code will need serious attention wrt. dealing with UTF-8 and,
// even worse, full-width characters.


using namespace BPackageKit;


typedef std::map<BSolverPackage*, BString> PackagePathMap;


static const char* kCommandUsage =
	"Usage: %s search <search-string>\n"
	"Searches for packages matching <search-string>.\n"
	"\n"
	"Options:\n"
	"  -i, --installed-only\n"
	"    Only find installed packages.\n"
	"  -u, --uninstalled-only\n"
	"    Only find not installed packages.\n"
	"\n"
;


static void
print_command_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kCommandUsage, kProgramName);
    exit(error ? 1 : 0);
}


static int
get_terminal_width()
{
    int fd = fileno(stdout);
    struct winsize windowSize;
	if (isatty(fd) == 1 && ioctl(fd, TIOCGWINSZ, &windowSize) == 0)
		return windowSize.ws_col;
	
    return INT_MAX;
}


int
command_search(int argc, const char* const* argv)
{
	bool installedOnly = false;
	bool uninstalledOnly = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "installed-only", no_argument, 0, 'i' },
			{ "uninstalled-only", no_argument, 0, 'u' },
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

			case 'i':
				installedOnly = true;
				uninstalledOnly = false;
				break;

			case 'u':
				uninstalledOnly = true;
				installedOnly = false;
				break;

			default:
				print_command_usage_and_exit(true);
				break;
		}
	}

	// The remaining argument is the search string.
	if (argc != optind + 1)
		print_command_usage_and_exit(true);

	const char* searchString = argv[optind++];

	// create the solver
	BSolver* solver;
	status_t error = BSolver::Create(solver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	// add repositories

	// installed
	BSolverRepository systemRepository;
	BSolverRepository commonRepository;
	BSolverRepository homeRepository;
	if (!uninstalledOnly) {
		RepositoryBuilder(systemRepository, "system")
			.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, "system")
			.AddToSolver(solver, false);
		RepositoryBuilder(commonRepository, "common")
			.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_COMMON, "common")
			.AddToSolver(solver, false);
//		RepositoryBuilder(homeRepository, "home")
//			.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_HOME, "home")
//			.AddToSolver(solver, false);
	}

	// not installed
	BObjectList<BSolverRepository> uninstalledRepositories(10, true);

	if (!installedOnly) {
		BPackageRoster roster;
		BStringList repositoryNames;
		error = roster.GetRepositoryNames(repositoryNames);
		if (error != B_OK)
			WARN(error, "failed to get repository names");

		int32 repositoryNameCount = repositoryNames.CountStrings();
		for (int32 i = 0; i < repositoryNameCount; i++) {
			const BString& name = repositoryNames.StringAt(i);
			BRepositoryConfig config;
			error = roster.GetRepositoryConfig(name, &config);
			if (error != B_OK) {
				WARN(error, "failed to get config for repository \"%s\". "
					"Skipping.", name.String());
				continue;
			}

			BSolverRepository* repository = new(std::nothrow) BSolverRepository;
			if (repository == NULL
				|| !uninstalledRepositories.AddItem(repository)) {
				DIE(B_NO_MEMORY, "out of memory");
			}

			RepositoryBuilder(*repository, config)
				.AddToSolver(solver, false);
		}
	}

	// search
	BObjectList<BSolverPackage> packages;
	error = solver->FindPackages(searchString,
		BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_SUMMARY
			| BSolver::B_FIND_IN_DESCRIPTION, packages);
	if (error != B_OK)
		DIE(error, "searching packages failed");

	if (packages.IsEmpty()) {
		printf("No matching packages found.\n");
		return 0;
	}

	// print table

	// determine column widths
	BString installedColumnTitle("Installed");
	BString nameColumnTitle("Name");
	BString descriptionColumnTitle("Description");

	int installedColumnWidth = installedColumnTitle.Length();
	int nameColumnWidth = nameColumnTitle.Length();
	int descriptionColumnWidth = descriptionColumnTitle.Length();

	int32 packageCount = packages.CountItems();
	for (int32 i = 0; i < packageCount; i++) {
		BSolverPackage* package = packages.ItemAt(i);
		nameColumnWidth = std::max(nameColumnWidth,
			(int)package->Name().Length());
		descriptionColumnWidth = std::max(descriptionColumnWidth,
			(int)package->Info().Summary().Length());
	}

	// print header
	BString header;
	header.SetToFormat("%-*s  %-*s  %s",
		installedColumnWidth, installedColumnTitle.String(),
		nameColumnWidth, nameColumnTitle.String(),
		descriptionColumnTitle.String());
	printf("%s\n", header.String());

	int minLineWidth = header.Length();
	int lineWidth = minLineWidth + descriptionColumnWidth
		- descriptionColumnTitle.Length();
	int terminalWidth = get_terminal_width();
	if (lineWidth > terminalWidth) {
		// truncate description
		int actualLineWidth = std::max(minLineWidth, terminalWidth);
		descriptionColumnWidth -= lineWidth - actualLineWidth;
		lineWidth = actualLineWidth;
	}

	header.SetTo('-', lineWidth);
	printf("%s\n", header.String());

	// print packages
	for (int32 i = 0; i < packageCount; i++) {
		BSolverPackage* package = packages.ItemAt(i);

		const char* installed = "";
		if (package->Repository() == &systemRepository)
			installed = "system";
		else if (package->Repository() == &commonRepository)
			installed = "common";
		else if (package->Repository() == &homeRepository)
			installed = "home";

		printf("%-*s  %-*s  %-*.*s\n",
			installedColumnWidth, installed,
			nameColumnWidth, package->Name().String(),
			descriptionColumnWidth, descriptionColumnWidth,
			package->Info().Summary().String());
	}

	return 0;
}
