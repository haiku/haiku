/*
 * Copyright 2013-2025, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Humdinger <humdingerb@gmail.com>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
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
	"  %command% [--show <field>] [-v] <package>\n"
	"    Shows information for the specified package.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% [--show <field>] [-v] <package>\n"
	"\n"
	"Shows information for the specified package.\n"
	"\n"
	"The <package> argument is the name by which the package\n"
	"is looked up in a remote repository.\n"
	"\n"
	"Options:\n"
	"    -s, --show field\n"
	"        List a specific metadata field from the given <package>.\n"
	"        The available fields are:\n"
	"        * header: package name and description (this is the default)\n"
	"        * provides: entities provided by the given package\n"
	"        * requires: runtime dependencies of the given package\n"
	"    -v, --versions\n"
	"        Show version details for the provided/required entities.\n"
	"        Otherwise only their names are printed.\n"
	;


DEFINE_COMMAND(InfoCommand, "info", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_PACKAGES)


int
InfoCommand::Execute(int argc, const char* const* argv)
{
	bool versions = false;
	BString field = "header";

	while (true) {
		static struct option sLongOptions[] = {
			{ "debug", required_argument, 0, OPTION_DEBUG },
			{ "help", no_argument, 0, 'h' },
			{ "show", required_argument, 0, 's' },
			{ "versions", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hs:v", sLongOptions, NULL);

		if (c == -1)
			break;

		if (fCommonOptions.HandleOption(c))
			continue;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			case 's':
				field = optarg;
				break;

			case 'v':
				versions = true;
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

	if ((field != "header") && (field != "provides") && (field != "requires"))
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

	// select first exact match by package name
	int32 packageCount = packages.CountItems();
	BSolverPackage* package = NULL;
	for (int32 i = 0; i < packageCount; i++) {
		if (packages.ItemAt(i)->Name() == packageName) {
			package = packages.ItemAt(i);
			break;
		}
	}

	if (package == NULL) {
		fprintf(stderr, "Could not find a package exactly named: \"%s\".\n", packageName);
		return 1;
	}

	if (field == "header") {
		// print out summary and description
		BString text("%name%: %summary%\n\n%description%\n");
		text.ReplaceFirst("%name%", package->Name());
		text.ReplaceFirst("%summary%", package->Info().Summary());
		text.ReplaceFirst("%description%", package->Info().Description());
		printf("%s\n", text.String());
		return 0;
	}

	if (field == "provides") {
		BObjectList<BPackageResolvable, true> providesList = package->Info().ProvidesList();
		for (int32 i = 0; i < providesList.CountItems(); i++) {
			if (versions)
				printf("%s\n", providesList.ItemAt(i)->ToString().String());
			else
				printf("%s\n", providesList.ItemAt(i)->Name().String());
		}
	}

	if (field == "requires") {
		BObjectList<BPackageResolvableExpression, true> requiresList
			= package->Info().RequiresList();
		for (int32 i = 0; i < requiresList.CountItems(); i++) {
			if (versions)
				printf("%s\n", requiresList.ItemAt(i)->ToString().String());
			else
				printf("%s\n", requiresList.ItemAt(i)->Name().String());
		}
	}

	return 0;
}
