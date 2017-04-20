/*
 * Copyright 2013-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Rene Gollent <rene@gollent.com>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Brian Hill <supernova@warpmail.net>
 */


#include "CheckManager.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <Catalog.h>
#include <Message.h>
#include <Messenger.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <NodeInfo.h>
#include <Notification.h>
#include <Roster.h>

#include <package/manager/Exceptions.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>

#include "constants.h"

using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CheckManager"


CheckManager::CheckManager(BPackageInstallationLocation location)
	:
	BPackageManager(location, &fClientInstallationInterface, this),
	BPackageManager::UserInteractionHandler(),
	fClientInstallationInterface()
{
}


void
CheckManager::CheckNetworkConnection()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;
	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		uint32 flags = interface.Flags();
		if ((flags & IFF_LOOPBACK) == 0
			&& (flags & (IFF_UP | IFF_LINK)) == (IFF_UP | IFF_LINK)) {
			return;
		}
	}
	
	// No network connection detected, cannot continue
	fprintf(stderr, B_TRANSLATE("No active network connection was found.\n"));
	throw BAbortedByUserException();
}


void
CheckManager::JobFailed(BSupportKit::BJob* job)
{
	BString error = job->ErrorString();
	if (error.Length() > 0) {
		error.ReplaceAll("\n", "\n*** ");
		fprintf(stderr, "%s", error.String());
	}
}


void
CheckManager::JobAborted(BSupportKit::BJob* job)
{
	printf("Job aborted\n");
	BString error = job->ErrorString();
	if (error.Length() > 0) {
		error.ReplaceAll("\n", "\n*** ");
		fprintf(stderr, "%s", error.String());
	}
}


void
CheckManager::HandleProblems()
{
	printf("Encountered problems:\n");

	int32 problemCount = fSolver->CountProblems();
	for (int32 i = 0; i < problemCount; i++) {
		BSolverProblem* problem = fSolver->ProblemAt(i);
		printf("problem %" B_PRId32 ": %s\n", i + 1,
			problem->ToString().String());
	}
	
	BString title(B_TRANSLATE("Available updates found"));
	BString text(B_TRANSLATE("Click here to run SoftwareUpdater. Some updates "
		"will require a problem solution to be selected."));
	_SendNotification(title, text);
	
	throw BAbortedByUserException();
}


void
CheckManager::ConfirmChanges(bool fromMostSpecific)
{
	int32 count = fInstalledRepositories.CountItems();
	int32 updateCount = 0;
	
	if (fromMostSpecific) {
		for (int32 i = count - 1; i >= 0; i--)
			_CountUpdates(*fInstalledRepositories.ItemAt(i), updateCount);
	} else {
		for (int32 i = 0; i < count; i++)
			_CountUpdates(*fInstalledRepositories.ItemAt(i), updateCount);
	}
	
	printf("Update count=%" B_PRId32 "\n", updateCount);
	if (updateCount > 0) {
		BString title(B_TRANSLATE("%count% packages have available updates"));
		BString count;
		count << updateCount;
		title.ReplaceFirst("%count%", count);
		BString text(B_TRANSLATE("Click here to install updates."));
		_SendNotification(title, text);
	}
	throw BAbortedByUserException();
}


void
CheckManager::Warn(status_t error, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	if (error == B_OK)
		printf("\n");
	else
		printf(": %s\n", strerror(error));
}


void
CheckManager::ProgressPackageDownloadStarted(const char* packageName)
{
	printf("Downloading %s...\n", packageName);
}


void
CheckManager::ProgressPackageDownloadActive(const char* packageName,
	float completionPercentage, off_t bytes, off_t totalBytes)
{
	static const char* progressChars[] = {
		"\xE2\x96\x8F",
		"\xE2\x96\x8E",
		"\xE2\x96\x8D",
		"\xE2\x96\x8C",
		"\xE2\x96\x8B",
		"\xE2\x96\x8A",
		"\xE2\x96\x89",
		"\xE2\x96\x88",
	};

	int width = 70;

	struct winsize winSize;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winSize) == 0
		&& winSize.ws_col < 77) {
		// We need 7 characters for the percent display
		width = winSize.ws_col - 7;
	}

	int position;
	int ipart = (int)(completionPercentage * width);
	int fpart = (int)(((completionPercentage * width) - ipart) * 8);

	printf("\r"); // return to the beginning of the line

	for (position = 0; position < width; position++) {
		if (position < ipart) {
			// This part is fully downloaded, show a full block
			printf(progressChars[7]);
		} else if (position > ipart) {
			// This part is not downloaded, show a space
			printf(" ");
		} else {
			// This part is partially downloaded
			printf(progressChars[fpart]);
		}
	}

	// Also print the progress percentage
	printf(" %3d%%", (int)(completionPercentage * 100));

	fflush(stdout);
	
}


void
CheckManager::ProgressPackageDownloadComplete(const char* packageName)
{
	// Overwrite the progress bar with whitespace
	printf("\r");
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	for (int i = 0; i < (w.ws_col); i++)
		printf(" ");
	printf("\r\x1b[1A"); // Go to previous line.

	printf("Downloading %s...done.\n", packageName);
}


void
CheckManager::ProgressPackageChecksumStarted(const char* title)
{
	printf("%s...", title);
}


void
CheckManager::ProgressPackageChecksumComplete(const char* title)
{
	printf("done.\n");
}


void
CheckManager::_CountUpdates(InstalledRepository& installationRepository,
	int32& updateCount)
{
	if (!installationRepository.HasChanges())
		return;

	PackageList& packagesToActivate
		= installationRepository.PackagesToActivate();
	PackageList& packagesToDeactivate
		= installationRepository.PackagesToDeactivate();

	for (int32 i = 0;
		BSolverPackage* installPackage = packagesToActivate.ItemAt(i);
		i++) {
		for (int32 j = 0;
			BSolverPackage* uninstallPackage = packagesToDeactivate.ItemAt(j);
			j++) {
			if (installPackage->Info().Name() == uninstallPackage->Info().Name()) {
				updateCount++;
				break;
			}
		}
	}
}


void
CheckManager::_SendNotification(const char* title, const char* text)
{
	BNotification notification(B_INFORMATION_NOTIFICATION);
	notification.SetGroup("SoftwareUpdater");
	notification.SetTitle(title);
	notification.SetContent(text);
	notification.SetOnClickApp(kAppSignature);
	BBitmap icon(_GetIcon());
	if (icon.IsValid())
		notification.SetIcon(&icon);
	bigtime_t timeout = 30000000;
	notification.Send(timeout);
}


BBitmap
CheckManager::_GetIcon()
{
	int32 iconSize = B_LARGE_ICON;
	BBitmap icon(BRect(0, 0, iconSize - 1, iconSize - 1), 0, B_RGBA32);
	team_info teamInfo;
	get_team_info(B_CURRENT_TEAM, &teamInfo);
	app_info appInfo;
	be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
	BNodeInfo::GetTrackerIcon(&appInfo.ref, &icon, icon_size(iconSize));
	return icon;
}
