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
	"  %command% [ <package> ... ]\n"
	"    Updates the specified or all packages.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% [ <package> ... ]\n"
	"Updates the specified packages. If no packages are given, all packages\n"
	"in the installation location are updated.\n"
	"\n"
	"Options:\n"
	"  -H, --home\n"
	"    Update the packages in the user's home directory. Default is to\n"
	"    update in the system directory.\n"
	"\n";


DEFINE_COMMAND(UpdateCommand, "update", kShortUsage, kLongUsage,
	kCommandCategoryPackages)


int
UpdateCommand::Execute(int argc, const char* const* argv)
{
	BPackageInstallationLocation location
		= B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "home", no_argument, 0, 'H' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hH", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			case 'H':
				location = B_PACKAGE_INSTALLATION_LOCATION_HOME;
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining arguments, if any, are the packages to be installed.
	int packageCount = argc - optind;
	const char* const* packages = argv + optind;

	// perform the update
	PackageManager packageManager(location);
	packageManager.Update(packages, packageCount);

	return 0;
}
