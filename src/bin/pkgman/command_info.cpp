/*
 * Copyright 2013-2023, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Humdinger <humdingerb@gmail.com>
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <package/solver/SolverPackage.h>

#include "Command.h"
#include "PackageManager.h"
#include "pkgman.h"


// TODO: internationalization!


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;


static const char* const kShortUsage =
	"  %command% <package>\n"
	"    Shows summary and description of the specified package.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% <package>\n"
	"Shows summary and description of the specified package.\n"
	"The <package> argument is the name by which the package\n"
	"is looked up in a remote repository.\n"
	"\n";


DEFINE_COMMAND(InfoCommand, "info", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_PACKAGES)


int
InfoCommand::Execute(int argc, const char* const* argv)
{
	while (true) {
		static struct option sLongOptions[] = {
			{ "debug", required_argument, 0, OPTION_DEBUG },
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "h", sLongOptions, NULL);

		if (c == -1)
			break;

		if (fCommonOptions.HandleOption(c))
			continue;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining argument, if any, is the package.
	const char* packageName = argv[optind++];
	if (packageName == NULL)
		PrintUsageAndExit(true);

	// create the solver
	PackageManager packageManager(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM);
	packageManager.SetDebugLevel(fCommonOptions.DebugLevel());
	packageManager.Init(
		PackageManager::B_ADD_INSTALLED_REPOSITORIES
		| PackageManager::B_ADD_REMOTE_REPOSITORIES);

	uint32 flags = BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME;

	// find packages
	BObjectList<BSolverPackage> packages;
	status_t error = packageManager.Solver()->FindPackages(packageName,
		flags, packages);
	if (error != B_OK)
		DIE(error, "searching packages failed");

	if (packages.IsEmpty()) {
		printf("No matching packages found.\n");
		return 0;
	}

	// print out summary and description of first exactly matching package
	int32 packageCount = packages.CountItems();
	for (int32 i = 0; i < packageCount; i++) {
		BSolverPackage* package = packages.ItemAt(i);
		if (package->Name() == packageName) {
			BString text("%name%: %summary%\n\n%description%\n");
			text.ReplaceFirst("%name%", package->Name());
			text.ReplaceFirst("%summary%", package->Info().Summary());
			text.ReplaceFirst("%description%", package->Info().Description());
			printf("%s\n", text.String());
			break;
		}
	}

	return 0;
}
