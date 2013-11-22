/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "Command.h"
#include "pkgman.h"
#include "PackageManager.h"


// TODO: internationalization!


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;


static const char* const kShortUsage =
	"  %command% <package> ...\n"
	"    Uninstalls one or more packages.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% <package> ...\n"
	"Uninstalls the specified packages.\n"
	"\n"
	"Options:\n"
	"  -H, --home\n"
	"    Uninstall the packages from the user's home directory. Default is to\n"
	"    uninstall from the system directory.\n"
	"  -y\n"
	"    Non-interactive mode. Automatically confirm changes, but fail when\n"
	"    encountering problems.\n"
	"\n";


DEFINE_COMMAND(UninstallCommand, "uninstall", kShortUsage, kLongUsage,
	kCommandCategoryPackages)


int
UninstallCommand::Execute(int argc, const char* const* argv)
{
	BPackageInstallationLocation location
		= B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;
	bool interactive = true;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "home", no_argument, 0, 'H' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hHy", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			case 'H':
				location = B_PACKAGE_INSTALLATION_LOCATION_HOME;
				break;

			case 'y':
				interactive = false;
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining arguments are the packages to be uninstalled.
	if (argc < optind + 1)
		PrintUsageAndExit(true);

	int packageCount = argc - optind;
	const char* const* packages = argv + optind;

	// perform the installation
	PackageManager packageManager(location, interactive);
	packageManager.Uninstall(packages, packageCount);

	return 0;
}
