/*
 * Copyright 2013-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Rene Gollent <rene@gollent.com>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Brian Hill <supernova@tycho.email>
 */


#include "UpdateManager.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Message.h>
#include <Messenger.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <Notification.h>
#include <Roster.h>

#include <package/manager/Exceptions.h>
#include <package/solver/SolverPackage.h>

#include "constants.h"
#include "ProblemWindow.h"

using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UpdateManager"


UpdateManager::UpdateManager(BPackageInstallationLocation location,
	bool verbose)
	:
	BPackageManager(location, &fClientInstallationInterface, this),
	BPackageManager::UserInteractionHandler(),
	fClientInstallationInterface(),
	fStatusWindow(NULL),
	fCurrentStep(ACTION_STEP_INIT),
	fChangesConfirmed(false),
	fVerbose(verbose)
{
	fStatusWindow = new SoftwareUpdaterWindow();
	_SetCurrentStep(ACTION_STEP_START);
}


UpdateManager::~UpdateManager()
{
	if (fStatusWindow != NULL) {
		fStatusWindow->Lock();
		fStatusWindow->Quit();
	}
	if (fProblemWindow != NULL) {
		fProblemWindow->Lock();
		fProblemWindow->Quit();
	}
}


void
UpdateManager::CheckNetworkConnection()
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
	throw BException(B_TRANSLATE_COMMENT(
		"No active network connection was found", "Error message"));
}


update_type
UpdateManager::GetUpdateType()
{
	int32 action = USER_SELECTION_NEEDED;
	BMessenger messenger(fStatusWindow);
	if (messenger.IsValid()) {
		BMessage message(kMsgGetUpdateType);
		BMessage reply;
		messenger.SendMessage(&message, &reply);
		reply.FindInt32(kKeyAlertResult, &action);
	}
	return (update_type)action;
}


void
UpdateManager::CheckRepositories()
{
	int32 count = fOtherRepositories.CountItems();
	if (fVerbose)
		printf("Remote repositories available: %" B_PRId32 "\n", count);
	if (count == 0) {
		BMessenger messenger(fStatusWindow);
		if (messenger.IsValid()) {
			BMessage message(kMsgNoRepositories);
			BMessage reply;
			messenger.SendMessage(&message, &reply);
			int32 result;
			reply.FindInt32(kKeyAlertResult, &result);
			if (result == 1)
				be_roster->Launch("application/x-vnd.Haiku-Repositories");
		}
		be_app->PostMessage(kMsgFinalQuit);
		throw BException(B_TRANSLATE_COMMENT(
			"No remote repositories are available", "Error message"));
	}
}


void
UpdateManager::JobFailed(BSupportKit::BJob* job)
{
	if (!fVerbose)
		return;
	
	BString error = job->ErrorString();
	if (error.Length() > 0) {
		error.ReplaceAll("\n", "\n*** ");
		fprintf(stderr, "%s", error.String());
	}
}


void
UpdateManager::JobAborted(BSupportKit::BJob* job)
{
	if (fVerbose)
		puts("Job aborted");
}


void
UpdateManager::FinalUpdate(const char* header, const char* text)
{
	_FinalUpdate(header, text);
}


void
UpdateManager::HandleProblems()
{
	if (fProblemWindow == NULL)
		fProblemWindow = new ProblemWindow;

	ProblemWindow::SolverPackageSet installPackages;
	ProblemWindow::SolverPackageSet uninstallPackages;
	if (!fProblemWindow->Go(fSolver,installPackages, uninstallPackages))
		throw BAbortedByUserException();
	fProblemWindow->Hide();
}


void
UpdateManager::ConfirmChanges(bool fromMostSpecific)
{
	if (fVerbose)
		puts("The following changes will be made:");

	int32 count = fInstalledRepositories.CountItems();
	int32 upgradeCount = 0;
	int32 installCount = 0;
	int32 uninstallCount = 0;
	
	if (fromMostSpecific) {
		for (int32 i = count - 1; i >= 0; i--)
			_PrintResult(*fInstalledRepositories.ItemAt(i), upgradeCount,
				installCount, uninstallCount);
	} else {
		for (int32 i = 0; i < count; i++)
			_PrintResult(*fInstalledRepositories.ItemAt(i), upgradeCount,
				installCount, uninstallCount);
	}
	
	if (fVerbose)
		printf("Upgrade count=%" B_PRId32 ", Install count=%" B_PRId32
			", Uninstall count=%" B_PRId32 "\n",
			upgradeCount, installCount, uninstallCount);
	
	fChangesConfirmed = fStatusWindow->ConfirmUpdates();
	if (!fChangesConfirmed)
		throw BAbortedByUserException();
	
	_SetCurrentStep(ACTION_STEP_DOWNLOAD);
	fPackageDownloadsTotal = upgradeCount + installCount;
	fPackageDownloadsCount = 1;
}


void
UpdateManager::Warn(status_t error, const char* format, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (fVerbose) {
		fputs(buffer, stderr);
		if (error == B_OK)
			puts("");
		else
			printf(": %s\n", strerror(error));
	}
	
	if (fStatusWindow != NULL) {
		if (fStatusWindow->UserCancelRequested())
			throw BAbortedByUserException();
		fStatusWindow->ShowWarningAlert(buffer);
	} else {
		BString text("SoftwareUpdater:\n");
		text.Append(buffer);
		BAlert* alert = new BAlert("warning", text, B_TRANSLATE("OK"), NULL,
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(NULL);
	}
}


void
UpdateManager::ProgressPackageDownloadStarted(const char* packageName)
{
	if (fCurrentStep == ACTION_STEP_DOWNLOAD) {
		BString header(B_TRANSLATE("Downloading packages"));
		_UpdateDownloadProgress(header.String(), packageName, 0.0);
		fNewDownloadStarted = false;
	}
	
	if (fVerbose)
		printf("Downloading %s...\n", packageName);
}


void
UpdateManager::ProgressPackageDownloadActive(const char* packageName,
	float completionValue, off_t bytes, off_t totalBytes)
{
	if (fCurrentStep == ACTION_STEP_DOWNLOAD) {
		// Fix a bug where a 100% completion percentage gets sent at the start
		// of a package download
		if (!fNewDownloadStarted) {
			if (completionValue > 0 && completionValue < 1)
				fNewDownloadStarted = true;
			else
				completionValue = 0.0;
		}
		_UpdateDownloadProgress(NULL, packageName, completionValue * 100.0);
	}

	if (fVerbose) {
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
		int ipart = (int)(completionValue * width);
		int fpart = (int)(((completionValue * width) - ipart) * 8);
	
		fputs("\r", stdout); // return to the beginning of the line
	
		for (position = 0; position < width; position++) {
			if (position < ipart) {
				// This part is fully downloaded, show a full block
				fputs(progressChars[7], stdout);
			} else if (position > ipart) {
				// This part is not downloaded, show a space
				fputs(" ", stdout);
			} else {
				// This part is partially downloaded
				fputs(progressChars[fpart], stdout);
			}
		}
	
		// Also print the progress percentage
		printf(" %3d%%", (int)(completionValue * 100));
	
		fflush(stdout);
	}
	
}


void
UpdateManager::ProgressPackageDownloadComplete(const char* packageName)
{
	if (fCurrentStep == ACTION_STEP_DOWNLOAD) {
		_UpdateDownloadProgress(NULL, packageName, 100.0);
		fPackageDownloadsCount++;
	}
	
	if (fVerbose) {
		// Overwrite the progress bar with whitespace
		fputs("\r", stdout);
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		for (int i = 0; i < (w.ws_col); i++)
			fputs(" ", stdout);
		fputs("\r\x1b[1A", stdout); // Go to previous line.
	
		printf("Downloading %s...done.\n", packageName);
	}
}


void
UpdateManager::ProgressPackageChecksumStarted(const char* title)
{
	// Repository checksums
	if (fCurrentStep == ACTION_STEP_START)
		_UpdateStatusWindow(NULL, title);
	
	if (fVerbose)
		printf("%s...", title);
}


void
UpdateManager::ProgressPackageChecksumComplete(const char* title)
{
	if (fVerbose)
		puts("done.");
}


void
UpdateManager::ProgressStartApplyingChanges(InstalledRepository& repository)
{
	_SetCurrentStep(ACTION_STEP_APPLY);
	BString header(B_TRANSLATE("Applying changes"));
	BString detail(B_TRANSLATE("Packages are being updated"));
	fStatusWindow->UpdatesApplying(header.String(), detail.String());
	
	if (fVerbose)
		printf("[%s] Applying changes ...\n", repository.Name().String());
}


void
UpdateManager::ProgressTransactionCommitted(InstalledRepository& repository,
	const BCommitTransactionResult& result)
{
	_SetCurrentStep(ACTION_STEP_COMPLETE);
	BString header(B_TRANSLATE("Updates completed"));
	BString detail(B_TRANSLATE("A reboot may be necessary to complete some "
		"updates."));
	_FinalUpdate(header.String(), detail.String());

	if (fVerbose) {
		const char* repositoryName = repository.Name().String();

		int32 issueCount = result.CountIssues();
		for (int32 i = 0; i < issueCount; i++) {
			const BTransactionIssue* issue = result.IssueAt(i);
			if (issue->PackageName().IsEmpty()) {
				printf("[%s] warning: %s\n", repositoryName,
					issue->ToString().String());
			} else {
				printf("[%s] warning: package %s: %s\n", repositoryName,
					issue->PackageName().String(), issue->ToString().String());
			}
		}
	
		printf("[%s] Changes applied. Old activation state backed up in \"%s\"\n",
			repositoryName, result.OldStateDirectory().String());
		printf("[%s] Cleaning up ...\n", repositoryName);
	}
}


void
UpdateManager::ProgressApplyingChangesDone(InstalledRepository& repository)
{
	if (fVerbose)
		printf("[%s] Done.\n", repository.Name().String());
}


void
UpdateManager::_PrintResult(InstalledRepository& installationRepository,
	int32& upgradeCount, int32& installCount, int32& uninstallCount)
{
	if (!installationRepository.HasChanges())
		return;

	if (fVerbose)
		printf("  in %s:\n", installationRepository.Name().String());

	PackageList& packagesToActivate
		= installationRepository.PackagesToActivate();
	PackageList& packagesToDeactivate
		= installationRepository.PackagesToDeactivate();

	BStringList upgradedPackages;
	BStringList upgradedPackageVersions;
	for (int32 i = 0;
		BSolverPackage* installPackage = packagesToActivate.ItemAt(i);
		i++) {
		for (int32 j = 0;
			BSolverPackage* uninstallPackage = packagesToDeactivate.ItemAt(j);
			j++) {
			if (installPackage->Info().Name() == uninstallPackage->Info().Name()) {
				upgradedPackages.Add(installPackage->Info().Name());
				upgradedPackageVersions.Add(uninstallPackage->Info().Version().ToString());
				break;
			}
		}
	}

	for (int32 i = 0; BSolverPackage* package = packagesToActivate.ItemAt(i);
		i++) {
		BString repository;
		if (dynamic_cast<MiscLocalRepository*>(package->Repository()) != NULL)
			repository = "local file";
		else
			repository.SetToFormat("repository %s",
				package->Repository()->Name().String());

		int position = upgradedPackages.IndexOf(package->Info().Name());
		if (position >= 0) {
			if (fVerbose)
				printf("    upgrade package %s-%s to %s from %s\n",
					package->Info().Name().String(),
					upgradedPackageVersions.StringAt(position).String(),
					package->Info().Version().ToString().String(),
					repository.String());
			fStatusWindow->AddPackageInfo(PACKAGE_UPDATE,
				package->Info().Name().String(),
				upgradedPackageVersions.StringAt(position).String(),
				package->Info().Version().ToString().String(),
				package->Info().Summary().String(),
				package->Repository()->Name().String(),
				package->Info().FileName().String());
			upgradeCount++;
		} else {
			if (fVerbose)
				printf("    install package %s-%s from %s\n",
					package->Info().Name().String(),
					package->Info().Version().ToString().String(),
					repository.String());
			fStatusWindow->AddPackageInfo(PACKAGE_INSTALL,
				package->Info().Name().String(),
				NULL,
				package->Info().Version().ToString().String(),
				package->Info().Summary().String(),
				package->Repository()->Name().String(),
				package->Info().FileName().String());
			installCount++;
		}
	}

	BStringList uninstallList;
	for (int32 i = 0; BSolverPackage* package = packagesToDeactivate.ItemAt(i);
		i++) {
		if (upgradedPackages.HasString(package->Info().Name()))
			continue;
		if (fVerbose)
			printf("    uninstall package %s\n",
				package->VersionedName().String());
		fStatusWindow->AddPackageInfo(PACKAGE_UNINSTALL,
			package->Info().Name().String(),
			package->Info().Version().ToString(),
			NULL,
			package->Info().Summary().String(),
			package->Repository()->Name().String(),
			package->Info().FileName().String());
		uninstallCount++;
	}
}


void
UpdateManager::_UpdateStatusWindow(const char* header, const char* detail)
{
	if (header == NULL && detail == NULL)
		return;
	
	if (fStatusWindow->UserCancelRequested())
		throw BAbortedByUserException();
	
	BMessage message(kMsgTextUpdate);
	if (header != NULL)
		message.AddString(kKeyHeader, header);
	if (detail != NULL)
		message.AddString(kKeyDetail, detail);
	fStatusWindow->PostMessage(&message);
}


void
UpdateManager::_UpdateDownloadProgress(const char* header,
	const char* packageName, float percentageComplete)
{
	if (packageName == NULL)
		return;
	
	if (fStatusWindow->UserCancelRequested())
		throw BAbortedByUserException();
	
	BString packageCount;
	packageCount.SetToFormat(
		B_TRANSLATE_COMMENT("%i of %i", "Do not translate %i"),
		fPackageDownloadsCount,
		fPackageDownloadsTotal);
	BMessage message(kMsgProgressUpdate);
	if (header != NULL)
		message.AddString(kKeyHeader, header);
	message.AddString(kKeyPackageName, packageName);
	message.AddString(kKeyPackageCount, packageCount.String());
	message.AddFloat(kKeyPercentage, percentageComplete);
	fStatusWindow->PostMessage(&message);
}


void
UpdateManager::_FinalUpdate(const char* header, const char* text)
{
	if (!fStatusWindow->IsFront()) {
		BNotification notification(B_INFORMATION_NOTIFICATION);
		notification.SetGroup("SoftwareUpdater");
		notification.SetTitle(header);
		notification.SetContent(text);
		notification.Send();
	}
	
	fStatusWindow->FinalUpdate(header, text);
}


void
UpdateManager::_SetCurrentStep(int32 step)
{
	fCurrentStep = step;
}
