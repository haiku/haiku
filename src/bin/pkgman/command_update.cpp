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
	"A <package> argument can be a search string by which the package is\n"
	"looked up locally and in a remote repository or a path to a local\n"
	"package file. In the latter case the file is copied.\n"
	"\n"
	"Options:\n"
	"  --debug <level>\n"
	"    Print debug output. <level> should be between 0 (no debug output,\n"
	"    the default) and 10 (most debug output).\n"
	"  -H, --home\n"
	"    Update the packages in the user's home directory. Default is to\n"
	"    update in the system directory.\n"
	"  -y\n"
	"    Non-interactive mode. Automatically confirm changes, but fail when\n"
	"    encountering problems.\n"
	"\n";


DEFINE_COMMAND(UpdateCommand, "update", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_PACKAGES)


int
UpdateCommand::Execute(int argc, const char* const* argv)
{
	BPackageInstallationLocation location
		= B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;
	bool interactive = true;

	while (true) {
		static struct option sLongOptions[] = {
			{ "debug", required_argument, 0, OPTION_DEBUG },
			{ "help", no_argument, 0, 'h' },
			{ "home", no_argument, 0, 'H' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hHy", sLongOptions, NULL);
		if (c == -1)
			break;

		if (fCommonOptions.HandleOption(c))
			continue;

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

	// The remaining arguments, if any, are the packages to be installed.
	int packageCount = argc - optind;
	const char* const* packages = argv + optind;

	// perform the update
	PackageManager packageManager(location, interactive);
	packageManager.SetDebugLevel(fCommonOptions.DebugLevel());
	packageManager.Update(packages, packageCount);

	return 0;
}
