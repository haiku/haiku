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

#include <algorithm>

#include <package/solver/SolverPackage.h>

#include "Command.h"
#include "PackageManager.h"
#include "pkgman.h"


// TODO: internationalization!
// The printing code will need serious attention wrt. dealing with UTF-8 and,
// even worse, full-width characters.


using namespace BPackageKit;


static const char* const kShortUsage =
	"  %command% ( <search-string> | --all | -a )\n"
	"    Searches for packages matching <search-string>.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% ( <search-string> | --all | -a )\n"
	"Searches for packages matching <search-string>.\n"
	"\n"
	"Options:\n"
	"  -a, --all\n"
	"    List all packages. Specified instead of <search-string>.\n"
	"  -i, --installed-only\n"
	"    Only find installed packages.\n"
	"  -u, --uninstalled-only\n"
	"    Only find not installed packages.\n"
	"\n";


DEFINE_COMMAND(SearchCommand, "search", kShortUsage, kLongUsage,
	kCommandCategoryPackages)


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
SearchCommand::Execute(int argc, const char* const* argv)
{
	bool installedOnly = false;
	bool uninstalledOnly = false;
	bool listAll = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "all", no_argument, 0, 'a' },
			{ "help", no_argument, 0, 'h' },
			{ "installed-only", no_argument, 0, 'i' },
			{ "uninstalled-only", no_argument, 0, 'u' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "ahiu", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'a':
				listAll = true;
				break;

			case 'h':
				PrintUsageAndExit(false);
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
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining argument is the search string. Ignored when --all has been
	// specified.
	if (!listAll && argc != optind + 1)
		PrintUsageAndExit(true);

	const char* searchString = listAll ? "" : argv[optind++];

	// create the solver
	PackageManager packageManager(B_PACKAGE_INSTALLATION_LOCATION_HOME);
	packageManager.Init(
		(!uninstalledOnly ? PackageManager::B_ADD_INSTALLED_REPOSITORIES : 0)
			| (!installedOnly ? PackageManager::B_ADD_REMOTE_REPOSITORIES : 0));

	// search
	BObjectList<BSolverPackage> packages;
	status_t error = packageManager.Solver()->FindPackages(searchString,
		BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME
			| BSolver::B_FIND_IN_SUMMARY | BSolver::B_FIND_IN_DESCRIPTION
			| BSolver::B_FIND_IN_PROVIDES,
		packages);
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
		if (package->Repository() == static_cast<const BSolverRepository*>(
				packageManager.SystemRepository()))
			installed = "system";
		else if (package->Repository() == static_cast<const BSolverRepository*>(
				packageManager.HomeRepository()))
			installed = "home";

		printf("%-*s  %-*s  %-*.*s\n",
			installedColumnWidth, installed,
			nameColumnWidth, package->Name().String(),
			descriptionColumnWidth, descriptionColumnWidth,
			package->Info().Summary().String());
	}

	return 0;
}
