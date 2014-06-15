/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 */


#include "PackageManager.h"

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
	fPreviousDownloadPercentage(0),
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
PackageManager::JobFailed(BJob* job)
{
	BString error = job->ErrorString();
	if (error.Length() > 0) {
		error.ReplaceAll("\n", "\n*** ");
		fprintf(stderr, "%s", error.String());
	}
}


void
PackageManager::JobAborted(BJob* job)
{
	DIE(job->Result(), "aborted");
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

	if (!fDecisionProvider.YesNoDecisionNeeded(BString(), "Continue?", "y", "n",
			"y")) {
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
	printf("Downloading %s...\n", packageName);
	fPreviousDownloadPercentage = 0;
}


void
PackageManager::ProgressPackageDownloadActive(const char* packageName,
	float completionPercentage)
{
	int32 currentPercentage = int32(completionPercentage * 100);
	int32 difference = currentPercentage - fPreviousDownloadPercentage;

	while (difference >= 2) {
		printf("#");
		difference -= 2;
	}
	fflush(stdout);

	fPreviousDownloadPercentage = currentPercentage - difference;
}


void
PackageManager::ProgressPackageDownloadComplete(const char* packageName)
{
	printf("\nFinished downloading %s.\n", packageName);
}


void
PackageManager::ProgressPackageChecksumStarted(const char* title)
{
	printf("%s...\n", title);
}


void
PackageManager::ProgressPackageChecksumComplete(const char* title)
{
	printf("%s complete.\n", title);
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

	for (int32 i = 0; BSolverPackage* package = packagesToActivate.ItemAt(i);
		i++) {
		if (dynamic_cast<MiscLocalRepository*>(package->Repository()) == NULL) {
			printf("    install package %s from repository %s\n",
				package->Info().FileName().String(),
				package->Repository()->Name().String());
		} else {
			printf("    install package %s from local file\n",
				package->Info().FileName().String());
		}
	}

	for (int32 i = 0; BSolverPackage* package = packagesToDeactivate.ItemAt(i);
		i++) {
		printf("    uninstall package %s\n", package->VersionedName().String());
	}
// TODO: Print file/download sizes. Unfortunately our package infos don't
// contain the file size. Which is probably correct. The file size (and possibly
// other information) should, however, be provided by the repository cache in
// some way. Extend BPackageInfo? Create a BPackageFileInfo?
}
