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

#include <package/PackageRoster.h>
#include <package/CleanUpAdminDirectoryRequest.h>
#include <Job.h>

#include "Command.h"
#include "DecisionProvider.h"
#include "pkgman.h"


// TODO: internationalization!


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;


static const char* const kShortUsage =
	"  %command% [-H]\n"
	"    Cleans up old states and transactions in the administrative directory.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% [-H]\n"
	"\n"
	"Cleans up old states and transactions in the administrative directory.\n"
	"\n"
	"Options:\n"
	"  -H, --home\n"
	"    Clean up the administrative directoryin the user's home directory.\n"
	"     Default is to clean up the system directory.\n"
	"  -y\n"
	"    Non-interactive mode. Automatically confirm changes, but fail when\n"
	"    encountering problems.\n"
	;


DEFINE_COMMAND(CleanupCommand, "cleanup", kShortUsage, kLongUsage,
	COMMAND_CATEGORY_OTHER)


int
CleanupCommand::Execute(int argc, const char* const* argv)
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

	BInstallationLocationInfo info;
	status_t status = BPackageRoster().GetInstallationLocationInfo(location, info);
	if (status == B_OK) {
		DecisionProvider decisionProvider(interactive);
		BSupportKit::BJobStateListener listener;
		BContext context(decisionProvider, listener);

		time_t before = time(NULL) - kCleanUpKeepDays * 24 * 60 * 60;
		CleanUpAdminDirectoryRequest request(context, info, before, kCleanUpKeepStates);
		status = request.Process();
	}

	return status;
}
