/*
 * Copyright 2013-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Rene Gollent <rene@gollent.com>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <StringForSize.h>
#include <StringForRate.h>
	// Must be first, or the BPrivate namespaces are confused

#include "PackageManager.h"

#include <InterfaceDefs.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include <package/CommitTransactionResult.h>
#include <package/DownloadFileRequest.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>

#include "pkgman.h"


using namespace BPackageKit::BPrivate;


PackageManager::PackageManager(BPackageInstallationLocation location,
	bool interactive)
	:
	BPackageManager(location, &fClientInstallationInterface, this),
	BPackageManager::UserInteractionHandler(),
	fDecisionProvider(interactive),
	fClientInstallationInterface(),
	fInteractive(interactive)
{
}


PackageManager::~PackageManager()
{
}


void
PackageManager::SetInteractive(bool interactive)
{
	fInteractive = interactive;
	fDecisionProvider.SetInteractive(interactive);
}


void
PackageManager::JobFailed(BSupportKit::BJob* job)
{
	BString error = job->ErrorString();
	if (error.Length() > 0) {
		error.ReplaceAll("\n", "\n*** ");
		fprintf(stderr, "%s", error.String());
	}
}


void
PackageManager::HandleProblems()
{
	printf("Encountered problems:\n");

	int32 problemCount = fSolver->CountProblems();
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

		if (!fInteractive)
			continue;

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
				exit(1);
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
	}

	if (problemCount > 0 && !fInteractive)
		exit(1);
}


void
PackageManager::ConfirmChanges(bool fromMostSpecific)
{
	printf("The following changes will be made:\n");

	int32 count = fInstalledRepositories.CountItems();
	if (fromMostSpecific) {
		for (int32 i = count - 1; i >= 0; i--)
			_PrintResult(*fInstalledRepositories.ItemAt(i));
	} else {
		for (int32 i = 0; i < count; i++)
			_PrintResult(*fInstalledRepositories.ItemAt(i));
	}

	if (!fDecisionProvider.YesNoDecisionNeeded(BString(), "Continue?", "yes",
			"no", "yes")) {
		exit(1);
	}
}


void
PackageManager::Warn(status_t error, const char* format, ...)
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
PackageManager::ProgressPackageDownloadStarted(const char* packageName)
{
	fShowProgress = isatty(STDOUT_FILENO);
	fLastBytes = 0;
	fLastRateCalcTime = system_time();
	fDownloadRate = 0;

	if (fShowProgress)
		printf("  0%%");
}


void
PackageManager::ProgressPackageDownloadActive(const char* packageName,
	float completionPercentage, off_t bytes, off_t totalBytes)
{
	if (bytes == totalBytes)
		fLastBytes = totalBytes;
	if (!fShowProgress)
		return;

	// Do not update if nothing changed in the last 500ms
	if (bytes <= fLastBytes || (system_time() - fLastRateCalcTime) < 500000)
		return;

	const bigtime_t time = system_time();
	if (time != fLastRateCalcTime) {
		fDownloadRate = (bytes - fLastBytes) * 1000000
			/ (time - fLastRateCalcTime);
	}
	fLastRateCalcTime = time;
	fLastBytes = bytes;

	// Build the current file progress percentage and size string
	BString leftStr;
	BString rightStr;

	int width = 70;
	struct winsize winSize;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winSize) == 0)
		width = std::min(winSize.ws_col - 2, 78);

	if (width < 30) {
		// Not much space for anything, just draw a percentage
		leftStr.SetToFormat("%3d%%", (int)(completionPercentage * 100));
	} else {
		leftStr.SetToFormat("%3d%% %s", (int)(completionPercentage * 100),
				packageName);

		char byteBuffer[32];
		char totalBuffer[32];
		char rateBuffer[32];
		rightStr.SetToFormat("%s/%s  %s ",
				string_for_size(bytes, byteBuffer, sizeof(byteBuffer)),
				string_for_size(totalBytes, totalBuffer, sizeof(totalBuffer)),
				fDownloadRate == 0 ? "--.-" :
				string_for_rate(fDownloadRate, rateBuffer, sizeof(rateBuffer)));

		if (leftStr.CountChars() + rightStr.CountChars() >= width)
		{
			// The full string does not fit! Try to make a shorter one.
			leftStr.ReplaceLast(".hpkg", "");
			leftStr.TruncateChars(width - rightStr.CountChars() - 2);
			leftStr.Append(B_UTF8_ELLIPSIS " ");
		}

		int extraSpace = width - leftStr.CountChars() - rightStr.CountChars();

		leftStr.Append(' ', extraSpace);
		leftStr.Append(rightStr);
	}

	const int progChars = leftStr.CountBytes(0,
		(int)(width * completionPercentage));

	// Set bg to green, fg to white, and print progress bar.
	// Then reset colors and print rest of text
	// And finally remove any stray chars at the end of the line
	printf("\r\x1B[42;37m%.*s\x1B[0m%s\x1B[K", progChars, leftStr.String(),
		leftStr.String() + progChars);

	// Force terminal to update when the line is complete, to avoid flickering
	// because of updates at random times
	fflush(stdout);
}


void
PackageManager::ProgressPackageDownloadComplete(const char* packageName)
{
	if (fShowProgress) {
		// Erase the line, return to the start, and reset colors
		printf("\r\33[2K\r\x1B[0m");
	}

	char byteBuffer[32];
	printf("100%% %s [%s]\n", packageName,
		string_for_size(fLastBytes, byteBuffer, sizeof(byteBuffer)));
	fflush(stdout);
}


void
PackageManager::ProgressPackageChecksumStarted(const char* title)
{
	printf("%s...", title);
}


void
PackageManager::ProgressPackageChecksumComplete(const char* title)
{
	printf("done.\n");
}


void
PackageManager::ProgressStartApplyingChanges(InstalledRepository& repository)
{
	printf("[%s] Applying changes ...\n", repository.Name().String());
}


void
PackageManager::ProgressTransactionCommitted(InstalledRepository& repository,
	const BCommitTransactionResult& result)
{
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
PackageManager::ProgressApplyingChangesDone(InstalledRepository& repository)
{
	printf("[%s] Done.\n", repository.Name().String());

	if (BPackageRoster().IsRebootNeeded())
		printf("A reboot is necessary to complete the installation process.\n");
}


void
PackageManager::_PrintResult(InstalledRepository& installationRepository)
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
			repository.SetToFormat("repository %s", package->Repository()->Name().String());

		int position = upgradedPackages.IndexOf(package->Info().Name());
		if (position >= 0) {
			printf("    upgrade package %s-%s to %s from %s\n",
				package->Info().Name().String(),
				upgradedPackageVersions.StringAt(position).String(),
				package->Info().Version().ToString().String(),
				repository.String());
		} else {
			printf("    install package %s-%s from %s\n",
				package->Info().Name().String(),
				package->Info().Version().ToString().String(),
				repository.String());
		}
	}

	for (int32 i = 0; BSolverPackage* package = packagesToDeactivate.ItemAt(i);
		i++) {
		if (upgradedPackages.HasString(package->Info().Name()))
			continue;
		printf("    uninstall package %s\n", package->VersionedName().String());
	}
// TODO: Print file/download sizes. Unfortunately our package infos don't
// contain the file size. Which is probably correct. The file size (and possibly
// other information) should, however, be provided by the repository cache in
// some way. Extend BPackageInfo? Create a BPackageFileInfo?
}
