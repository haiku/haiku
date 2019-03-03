/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <getopt.h>
#include <package/manager/Exceptions.h>
#include <ObjectList.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <stdio.h>
#include <stdlib.h>

#include "Command.h"
#include "pkgman.h"
#include "PackageManager.h"


// TODO: internationalization!


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;


static const char* const kShortUsage =
	"  %command% <package> ...\n"
	"    Installs one or more packages.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% <package> ...\n"
	"Installs the specified packages. A <package> argument can be a search\n"
	"string by which the package is looked up in a remote repository or a\n"
	"path to a local package file. In the latter case the file is copied.\n"
	"\n"
	"Options:\n"
	"  --debug <level>\n"
	"    Print debug output. <level> should be between 0 (no debug output,\n"
	"    the default) and 10 (most debug output).\n"
	"  -H, --home\n"
	"    Install the packages in the user's home directory. Default is to\n"
	"    install in the system directory.\n"
	"  -y\n"
	"    Non-interactive mode. Automatically confirm changes, but fail when\n"
	"    encountering problems.\n"
	"\n";


DEFINE_COMMAND(InstallCommand, "install", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_PACKAGES)


int
InstallCommand::Execute(int argc, const char* const* argv)
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

	// The remaining arguments are the packages to be installed.
	if (argc < optind + 1)
		PrintUsageAndExit(true);

	int packageCount = argc - optind;
	const char* const* packages = argv + optind;

	// perform the installation
	PackageManager packageManager(location, interactive);
	packageManager.SetDebugLevel(fCommonOptions.DebugLevel());
	try {
		packageManager.Install(packages, packageCount);
	} catch (BNothingToDoException&) {
		// Output already installed packages
		BSolverPackageSpecifierList packagesToInstall;
		if (!packagesToInstall.AppendSpecifiers(packages, packageCount))
			throw std::bad_alloc();
		// Find the installed packages that match the specification
		const BSolverPackageSpecifier* unmatchedSpecifier;
		BObjectList<BSolverPackage> installedPackages;
		packageManager.Solver()->FindPackages(packagesToInstall,
			BSolver::B_FIND_INSTALLED_ONLY,
			installedPackages, &unmatchedSpecifier);

		for (int32 i = 0; BSolverPackage* package = installedPackages.ItemAt(i);
			i++) {
			BString repository;
			if (dynamic_cast<PackageManager::MiscLocalRepository*>(
					package->Repository()) != NULL)
				repository = "local file";
			else
				repository.SetToFormat(
					"repository %s", package->Repository()->Name().String());
			fprintf(stderr, "%s from %s is already installed.\n",
				package->VersionedName().String(),
				repository.String());
		}
		throw;
	}

	return 0;
}
