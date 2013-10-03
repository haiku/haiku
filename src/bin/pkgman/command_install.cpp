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
	"    Installs one or more packages.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% <package> ...\n"
	"Installs the specified packages.\n"
	"\n"
	"Options:\n"
	"  -H, --home\n"
	"    Install the packages in the user's home directory. Default is to\n"
	"    install in the system directory.\n"
	"\n";


DEFINE_COMMAND(InstallCommand, "install", kShortUsage, kLongUsage,
	kCommandCategoryPackages)


int
InstallCommand::Execute(int argc, const char* const* argv)
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

	// The remaining arguments are the packages to be installed.
	if (argc < optind + 1)
		PrintUsageAndExit(true);

	int packageCount = argc - optind;
	const char* const* packages = argv + optind;

	// perform the installation
	PackageManager packageManager(location);
	packageManager.Install(packages, packageCount);

	return 0;
}
