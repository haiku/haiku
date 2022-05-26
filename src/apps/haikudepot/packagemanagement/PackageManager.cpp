/*
 * Copyright 2013-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 * 		Stephan Aßmus <superstippi@gmx.de>
 * 		Rene Gollent <rene@gollent.com>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Andrew Lindesay <apl@lindesay.co.nz>
 */


#include "PackageManager.h"

#include <Alert.h>
#include <Catalog.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>

#include <package/DownloadFileRequest.h>
#include <package/manager/Exceptions.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/PackageReader.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "Logger.h"
#include "OpenPackageProcess.h"
#include "PackageInfo.h"
#include "ProblemWindow.h"
#include "ResultWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageManager"


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;

using BPackageKit::BRefreshRepositoryRequest;
using BPackageKit::DownloadFileRequest;
using BPackageKit::BSolver;
using BPackageKit::BSolverPackage;
using BPackageKit::BSolverRepository;
using BPackageKit::BHPKG::BNoErrorOutput;
using BPackageKit::BHPKG::BPackageContentHandler;
using BPackageKit::BHPKG::BPackageEntry;
using BPackageKit::BHPKG::BPackageEntryAttribute;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BPackageReader;


// #pragma mark - PackageProgressListener


PackageProgressListener::~PackageProgressListener()
{
}


void
PackageProgressListener::DownloadProgressChanged(const char* packageName,
	float progress)
{
}


void
PackageProgressListener::DownloadProgressComplete(const char* packageName)
{
}


void
PackageProgressListener::ConfirmedChanges(
	BPackageManager::InstalledRepository& repository)
{
}


void
PackageProgressListener::StartApplyingChanges(
	BPackageManager::InstalledRepository& repository)
{
}


void
PackageProgressListener::ApplyingChangesDone(
	BPackageManager::InstalledRepository& repository)
{
}


// #pragma mark - PackageManager


PackageManager::PackageManager(BPackageInstallationLocation location)
	:
	BPackageManager(location, &fClientInstallationInterface, this),
	BPackageManager::UserInteractionHandler(),
	fDecisionProvider(),
	fClientInstallationInterface(),
	fProblemWindow(NULL),
	fCurrentInstallPackage(NULL),
	fCurrentUninstallPackage(NULL)
{
}


PackageManager::~PackageManager()
{
	if (fProblemWindow != NULL)
		fProblemWindow->PostMessage(B_QUIT_REQUESTED);
}


PackageState
PackageManager::GetPackageState(const PackageInfo& package)
{
	// TODO: Fetch information from the PackageKit
	return NONE;
}


void
PackageManager::CollectPackageActions(PackageInfoRef package,
		Collector<PackageActionRef>& actionList)
{
	if (package->IsSystemPackage() || package->IsSystemDependency())
		return;

	switch (package->State()) {
		case ACTIVATED:
		case INSTALLED:
			_CollectPackageActionsForActivatedOrInstalled(package, actionList);
			break;
		case NONE:
		case UNINSTALLED:
			actionList.Add(_CreateInstallPackageAction(package));
			break;
		case DOWNLOADING:
			HDINFO("no package actions for [%s] (downloading)",
				package->Name().String());
			break;
		case PENDING:
			HDINFO("no package actions for [%s] (pending)",
				package->Name().String());
			break;
		default:
			HDFATAL("unexpected status for package [%s]",
				package->Name().String());
			break;
	}
}


void
PackageManager::_CollectPackageActionsForActivatedOrInstalled(
		PackageInfoRef package,
		Collector<PackageActionRef>& actionList)
{
	actionList.Add(_CreateUninstallPackageAction(package));

	// Add OpenPackageActions for each deskbar link found in the
	// package
	std::vector<DeskbarLink> foundLinks;
	if (OpenPackageProcess::FindAppToLaunch(package, foundLinks)
		&& foundLinks.size() < 4) {
		std::vector<DeskbarLink>::const_iterator it;
		for (it = foundLinks.begin(); it != foundLinks.end(); it++) {
			const DeskbarLink& aLink = *it;
			actionList.Add(_CreateOpenPackageAction(package, aLink));
		}
	}
}


PackageActionRef
PackageManager::_CreateUninstallPackageAction(const PackageInfoRef& package)
{
	BString title = B_TRANSLATE("Uninstall %PackageTitle%");
	title.ReplaceAll("%PackageTitle%", package->Title());

	BMessage message(MSG_PKG_UNINSTALL);
	message.AddString(KEY_TITLE, title);
	message.AddString(KEY_PACKAGE_NAME, package->Name());

	return PackageActionRef(new PackageAction(title, message));
}


PackageActionRef
PackageManager::_CreateInstallPackageAction(const PackageInfoRef& package)
{
	BString title = B_TRANSLATE("Install %PackageTitle%");
	title.ReplaceAll("%PackageTitle%", package->Title());

	BMessage message(MSG_PKG_INSTALL);
	message.AddString(KEY_TITLE, title);
	message.AddString(KEY_PACKAGE_NAME, package->Name());

	return PackageActionRef(new PackageAction(title, message));
}


PackageActionRef
PackageManager::_CreateOpenPackageAction(const PackageInfoRef& package,
	const DeskbarLink& link)
{
	BPath linkPath(link.Link());
	BString title = B_TRANSLATE("Open %DeskbarLink%");
	title.ReplaceAll("%DeskbarLink%", linkPath.Leaf());

	BMessage deskbarLinkMessage;
	if (link.Archive(&deskbarLinkMessage) != B_OK)
		HDFATAL("unable to archive the deskbar link");

	BMessage message(MSG_PKG_OPEN);
	message.AddString(KEY_TITLE, title);
	message.AddMessage(KEY_DESKBAR_LINK, &deskbarLinkMessage);
	message.AddString(KEY_PACKAGE_NAME, package->Name());

	return PackageActionRef(new PackageAction(title, message));
}


void
PackageManager::SetCurrentActionPackage(PackageInfoRef package, bool install)
{
	BSolverPackage* solverPackage = _GetSolverPackage(package);
	fCurrentInstallPackage = install ? solverPackage : NULL;
	fCurrentUninstallPackage = install ? NULL : solverPackage;
}


status_t
PackageManager::RefreshRepository(const BRepositoryConfig& repoConfig)
{
	status_t result;
	try {
		result = BPackageManager::RefreshRepository(repoConfig);
	} catch (BFatalErrorException& ex) {
		HDERROR("Fatal error occurred while refreshing repository: "
			"%s (%s)", ex.Message().String(), ex.Details().String());
		result = ex.Error();
	} catch (BException& ex) {
		HDERROR("Exception occurred while refreshing "
			"repository: %s\n", ex.Message().String());
		result = B_ERROR;
	}

	return result;
}


status_t
PackageManager::DownloadPackage(const BString& fileURL,
	const BEntry& targetEntry, const BString& checksum)
{
	status_t result;
	try {
		result = BPackageManager::DownloadPackage(fileURL, targetEntry,
			checksum);
	} catch (BFatalErrorException& ex) {
		HDERROR("Fatal error occurred while downloading package: "
			"%s: %s (%s)", fileURL.String(), ex.Message().String(),
			ex.Details().String());
		result = ex.Error();
	} catch (BException& ex) {
		HDERROR("Exception occurred while downloading package "
			"%s: %s", fileURL.String(), ex.Message().String());
		result = B_ERROR;
	}

	return result;
}


void
PackageManager::AddProgressListener(PackageProgressListener* listener)
{
	fPackageProgressListeners.AddItem(listener);
}


void
PackageManager::RemoveProgressListener(PackageProgressListener* listener)
{
	fPackageProgressListeners.RemoveItem(listener);
}


void
PackageManager::HandleProblems()
{
	if (fProblemWindow == NULL)
		fProblemWindow = new ProblemWindow;

	ProblemWindow::SolverPackageSet installPackages;
	ProblemWindow::SolverPackageSet uninstallPackages;
	if (fCurrentInstallPackage != NULL)
		installPackages.insert(fCurrentInstallPackage);

	if (fCurrentUninstallPackage != NULL)
		uninstallPackages.insert(fCurrentUninstallPackage);

	if (!fProblemWindow->Go(fSolver,installPackages, uninstallPackages))
		throw BAbortedByUserException();
}


void
PackageManager::ConfirmChanges(bool fromMostSpecific)
{
	ResultWindow* window = new ResultWindow;
	ObjectDeleter<ResultWindow> windowDeleter(window);

	bool hasOtherChanges = false;
	int32 count = fInstalledRepositories.CountItems();

	if (fromMostSpecific) {
		for (int32 i = count - 1; i >= 0; i--)
			hasOtherChanges
				|= _AddResults(*fInstalledRepositories.ItemAt(i), window);
	} else {
		for (int32 i = 0; i < count; i++)
			hasOtherChanges
				|= _AddResults(*fInstalledRepositories.ItemAt(i), window);
	}

	if (!hasOtherChanges) {
		_NotifyChangesConfirmed();
		return;
	}

	// show the window
	if (windowDeleter.Detach()->Go() == 0)
		throw BAbortedByUserException();

	_NotifyChangesConfirmed();
}


void
PackageManager::Warn(status_t error, const char* format, ...)
{
	// TODO: Show alert to user

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
	ProgressPackageDownloadActive(packageName, 0.0f, 0, 0);
}


void
PackageManager::ProgressPackageDownloadActive(const char* packageName,
	float completionPercentage, off_t bytes, off_t totalBytes)
{
	for (int32 i = 0; i < fPackageProgressListeners.CountItems(); i++) {
		fPackageProgressListeners.ItemAt(i)->DownloadProgressChanged(
			packageName, completionPercentage);
	}
}


void
PackageManager::ProgressPackageDownloadComplete(const char* packageName)
{
	for (int32 i = 0; i < fPackageProgressListeners.CountItems(); i++) {
		fPackageProgressListeners.ItemAt(i)->DownloadProgressComplete(
			packageName);
	}
}


void
PackageManager::ProgressPackageChecksumStarted(const char* title)
{
	// TODO: implement
}


void
PackageManager::ProgressPackageChecksumComplete(const char* title)
{
	// TODO: implement
}


void
PackageManager::ProgressStartApplyingChanges(InstalledRepository& repository)
{
	for (int32 i = 0; i < fPackageProgressListeners.CountItems(); i++)
		fPackageProgressListeners.ItemAt(i)->StartApplyingChanges(repository);
}


void
PackageManager::ProgressTransactionCommitted(InstalledRepository& repository,
	const BCommitTransactionResult& result)
{
	// TODO: implement
}


void
PackageManager::ProgressApplyingChangesDone(InstalledRepository& repository)
{
	for (int32 i = 0; i < fPackageProgressListeners.CountItems(); i++)
		fPackageProgressListeners.ItemAt(i)->ApplyingChangesDone(repository);

	if (BPackageRoster().IsRebootNeeded()) {
		BString infoString(B_TRANSLATE("A reboot is necessary to complete the "
			"installation process."));
		BAlert* alert = new(std::nothrow) BAlert(B_TRANSLATE("Reboot required"),
			infoString, B_TRANSLATE("Close"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_INFO_ALERT);
		if (alert != NULL)
			alert->Go();
	}
}


bool
PackageManager::_AddResults(InstalledRepository& repository,
	ResultWindow* window)
{
	if (!repository.HasChanges())
		return false;

	ProblemWindow::SolverPackageSet installPackages;
	ProblemWindow::SolverPackageSet uninstallPackages;
	if (fCurrentInstallPackage != NULL)
		installPackages.insert(fCurrentInstallPackage);

	if (fCurrentUninstallPackage != NULL)
		uninstallPackages.insert(fCurrentUninstallPackage);

	return window->AddLocationChanges(repository.Name(),
		repository.PackagesToActivate(), installPackages,
		repository.PackagesToDeactivate(), uninstallPackages);
}


void
PackageManager::_NotifyChangesConfirmed()
{
	int32 count = fInstalledRepositories.CountItems();
	for (int32 i = 0; i < count; i++) {
		for (int32 j = 0; j < fPackageProgressListeners.CountItems(); j++) {
			fPackageProgressListeners.ItemAt(j)->ConfirmedChanges(
				*fInstalledRepositories.ItemAt(i));
		}
	}
}


BSolverPackage*
PackageManager::_GetSolverPackage(PackageInfoRef package)
{
	int32 flags = BSolver::B_FIND_IN_NAME;
	if (package->State() == ACTIVATED || package->State() == INSTALLED)
		flags |= BSolver::B_FIND_INSTALLED_ONLY;

	BObjectList<BSolverPackage> packages;
	status_t result = Solver()->FindPackages(package->Name(), flags, packages);
	if (result == B_OK) {
		for (int32 i = 0; i < packages.CountItems(); i++) {
			BSolverPackage* solverPackage = packages.ItemAt(i);
			if (solverPackage->Name() != package->Name())
				continue;
			else if (package->State() == NONE
				&& dynamic_cast<BPackageManager::RemoteRepository*>(
					solverPackage->Repository()) == NULL) {
				continue;
			}
			return solverPackage;
		}
	}

	return NULL;
}
