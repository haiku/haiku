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


#include "UpdateManager.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <Alert.h>
#include <Catalog.h>
#include <Notification.h>

#include <package/CommitTransactionResult.h>
#include <package/DownloadFileRequest.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/manager/Exceptions.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>

#include "constants.h"
#include "AutoDeleter.h"
#include "ProblemWindow.h"
#include "ResultWindow.h"

using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UpdateManager"


UpdateManager::UpdateManager(BPackageInstallationLocation location)
	:
	BPackageManager(location, &fClientInstallationInterface, this),
	BPackageManager::UserInteractionHandler(),
	fClientInstallationInterface(),
	fStatusWindow(NULL),
	fFinalWindow(NULL),
	fCurrentStep(ACTION_STEP_INIT),
	fChangesConfirmed(false)
{
	fStatusWindow = new SoftwareUpdaterWindow();
	_SetCurrentStep(ACTION_STEP_START);
}


UpdateManager::~UpdateManager()
{
	if (fStatusWindow != NULL)
		fStatusWindow->PostMessage(B_QUIT_REQUESTED);
	if (fFinalWindow != NULL)
		fFinalWindow->PostMessage(B_QUIT_REQUESTED);
	if (fProblemWindow != NULL)
		fProblemWindow->PostMessage(B_QUIT_REQUESTED);
}


void
UpdateManager::JobFailed(BSupportKit::BJob* job)
{
	BString error = job->ErrorString();
	if (error.Length() > 0) {
		error.ReplaceAll("\n", "\n*** ");
		fprintf(stderr, "%s", error.String());
	}
}


void
UpdateManager::JobAborted(BSupportKit::BJob* job)
{
	//DIE(job->Result(), "aborted");
	printf("Job aborted\n");
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

/*	int32 problemCount = fSolver->CountProblems();
	for (int32 i = 0; i < problemCount; i++) {
		// print problem and possible solutions
		BSolverProblem* problem = fSolver->ProblemAt(i);
		printf("problem %" B_PRId32 ": %s\n", i + 1,
			problem->ToString().String());

		int32 solutionCount = problem->CountSolutions();
		for (int32 k = 0; k < solutionCount; k++) {
			const BSolverProblemSolution* solution = problem->SolutionAt(k);
			printf("  solution %" B_PRId32 ":\n", k + 1);
			int32 elementCount = solution->CountElements();
			for (int32 l = 0; l < elementCount; l++) {
				const BSolverProblemSolutionElement* element
					= solution->ElementAt(l);
				printf("    - %s\n", element->ToString().String());
			}
		}

		// let the user choose a solution
		printf("Please select a solution, skip the problem for now or quit.\n");
		for (;;) {
			if (solutionCount > 1)
				printf("select [1...%" B_PRId32 "/s/q]: ", solutionCount);
			else
				printf("select [1/s/q]: ");

			char buffer[32];
			if (fgets(buffer, sizeof(buffer), stdin) == NULL
				|| strcmp(buffer, "q\n") == 0) {
				//exit(1);
			}

			if (strcmp(buffer, "s\n") == 0)
				break;

			
			char* end;
			long selected = strtol(buffer, &end, 0);
			if (end == buffer || *end != '\n' || selected < 1
				|| selected > solutionCount) {
				printf("*** invalid input\n");
				continue;
			}

			status_t error = fSolver->SelectProblemSolution(problem,
				problem->SolutionAt(selected - 1));
			if (error != B_OK)
				DIE(error, "failed to set solution");
			
			break;
		}
	}*/
}


void
UpdateManager::ConfirmChanges(bool fromMostSpecific)
{
	printf("The following changes will be made:\n");

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
	
	printf("Upgrade count=%" B_PRId32 ", Install count=%" B_PRId32
		", Uninstall count=%" B_PRId32 "\n",
		upgradeCount, installCount, uninstallCount);
	BString text;
	if (upgradeCount == 1)
		text.SetTo(B_TRANSLATE_COMMENT("There is 1 update "
		"%dependancies%available.",
		"Do not translate %dependancies%"));
	else
		text.SetTo(B_TRANSLATE_COMMENT("There are %count% updates "
		"%dependancies%available.",
		"Do not translate %count% or %dependancies%"));
	BString countString;
	countString << upgradeCount;
	text.ReplaceFirst("%count%", countString);
	BString dependancies("");
	if (installCount) {
		dependancies.SetTo("(");
		dependancies.Append(B_TRANSLATE("with")).Append(" ");
		if (installCount == 1)
			dependancies.Append(B_TRANSLATE("1 new dependancy"));
		else {
			dependancies << installCount;
			dependancies.Append(" ").Append(B_TRANSLATE("new dependancies"));
		}
		dependancies.Append(") ");
	}
	text.ReplaceFirst("%dependancies%", dependancies);
	fChangesConfirmed = fStatusWindow->ConfirmUpdates(text.String());
	if (!fChangesConfirmed)
		throw BAbortedByUserException();
	
	_SetCurrentStep(ACTION_STEP_DOWNLOAD);
	fPackageDownloadsTotal = upgradeCount + installCount;
	fPackageDownloadsCount = 1;
}


void
UpdateManager::Warn(status_t error, const char* format, ...)
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
UpdateManager::ProgressPackageDownloadStarted(const char* packageName)
{
	if (fCurrentStep == ACTION_STEP_DOWNLOAD) {
		BString header(B_TRANSLATE("Downloading packages"));
		BString packageCount;
		packageCount.SetToFormat(
			B_TRANSLATE_COMMENT("%i of %i", "Do not translate %i"),
			fPackageDownloadsCount,
			fPackageDownloadsTotal);
		_UpdateDownloadProgress(header.String(), packageName, packageCount,
			0.0);
		fNewDownloadStarted = false;
	}
	
	printf("Downloading %s...\n", packageName);
}


void
UpdateManager::ProgressPackageDownloadActive(const char* packageName,
	float completionPercentage, off_t bytes, off_t totalBytes)
{
	if (fCurrentStep == ACTION_STEP_DOWNLOAD) {
		// Fix a bug where a 100% completion percentage gets sent at the start
		// of a package download
		if (!fNewDownloadStarted) {
			if (completionPercentage > 0 && completionPercentage < 1)
				fNewDownloadStarted = true;
			else
				completionPercentage = 0.0;
		}
		BString packageCount;
		packageCount.SetToFormat(
			B_TRANSLATE_COMMENT("%i of %i", "Do not translate %i"),
			fPackageDownloadsCount,
			fPackageDownloadsTotal);
		_UpdateDownloadProgress(NULL, packageName, packageCount,
			completionPercentage);
	}

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
UpdateManager::ProgressPackageDownloadComplete(const char* packageName)
{
	if (fCurrentStep == ACTION_STEP_DOWNLOAD)
		fPackageDownloadsCount++;
	
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
UpdateManager::ProgressPackageChecksumStarted(const char* title)
{
	// Repository checksums
	if (fCurrentStep == ACTION_STEP_START)
		_UpdateStatusWindow(NULL, title);
	
	printf("%s...", title);
}


void
UpdateManager::ProgressPackageChecksumComplete(const char* title)
{
	printf("done.\n");
}


void
UpdateManager::ProgressStartApplyingChanges(InstalledRepository& repository)
{
	_SetCurrentStep(ACTION_STEP_APPLY);
	BString header(B_TRANSLATE("Applying changes"));
	BString detail(B_TRANSLATE("Packages are being updated"));
	fStatusWindow->UpdatesApplying(header.String(), detail.String());
	
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


void
UpdateManager::ProgressApplyingChangesDone(InstalledRepository& repository)
{
	printf("[%s] Done.\n", repository.Name().String());
}


void
UpdateManager::_PrintResult(InstalledRepository& installationRepository,
	int32& upgradeCount, int32& installCount, int32& uninstallCount)
{
	if (!installationRepository.HasChanges())
		return;

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
				package->Repository()->Name().String());
			upgradeCount++;
		} else {
			printf("    install package %s-%s from %s\n",
				package->Info().Name().String(),
				package->Info().Version().ToString().String(),
				repository.String());
			fStatusWindow->AddPackageInfo(PACKAGE_INSTALL,
				package->Info().Name().String(),
				NULL,
				package->Info().Version().ToString().String(),
				package->Info().Summary().String(),
				package->Repository()->Name().String());
			installCount++;
		}
	}

	BStringList uninstallList;
	for (int32 i = 0; BSolverPackage* package = packagesToDeactivate.ItemAt(i);
		i++) {
		if (upgradedPackages.HasString(package->Info().Name()))
			continue;
		printf("    uninstall package %s\n", package->VersionedName().String());
		fStatusWindow->AddPackageInfo(PACKAGE_UNINSTALL,
			package->Info().Name().String(),
			package->Info().Version().ToString(),
			NULL,
			package->Info().Summary().String(),
			package->Repository()->Name().String());
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
	const char* packageName, const char* packageCount,
	float completionPercentage)
{
	if (packageName == NULL || packageCount == NULL)
		return;
	
	if (fStatusWindow->UserCancelRequested())
		throw BAbortedByUserException();
	
	BMessage message(kMsgProgressUpdate);
	if (header != NULL)
		message.AddString(kKeyHeader, header);
	message.AddString(kKeyPackageName, packageName);
	message.AddString(kKeyPackageCount, packageCount);
	message.AddFloat(kKeyPercentage, completionPercentage);
	fStatusWindow->PostMessage(&message);
}


void
UpdateManager::_FinalUpdate(const char* header, const char* text)
{
	BNotification notification(B_INFORMATION_NOTIFICATION);
	notification.SetGroup("SoftwareUpdater");
	notification.SetTitle(header);
	notification.SetContent(text);
	const BBitmap* icon = fStatusWindow->GetIcon();
	if (icon != NULL)
		notification.SetIcon(icon);
	notification.Send();
	
	BPoint location(fStatusWindow->GetLocation());
	BRect rect(fStatusWindow->GetDefaultRect());
	fStatusWindow->PostMessage(B_QUIT_REQUESTED);
	fStatusWindow = NULL;
	
	fFinalWindow = new FinalWindow(rect, location, header, text);
}

void
UpdateManager::_SetCurrentStep(int32 step)
{
	fCurrentStep = step;
}
