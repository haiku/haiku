/*
 * Copyright 2013-2020, Haiku, Inc. All Rights Reserved.
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
#include "Model.h"
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


typedef std::set<PackageInfoRef> PackageInfoSet;


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



// #pragma mark - InstallPackageAction


class InstallPackageAction : public PackageAction,
	private PackageProgressListener {
public:
	InstallPackageAction(PackageInfoRef package, Model* model)
		:
		PackageAction(PACKAGE_ACTION_INSTALL, package, model),
		fLastDownloadUpdate(0)
	{
	}

	virtual const char* Label() const
	{
		return B_TRANSLATE("Install");
	}

	virtual status_t Perform()
	{
		fPackageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
			| BPackageManager::B_ADD_REMOTE_REPOSITORIES
			| BPackageManager::B_REFRESH_REPOSITORIES);
		PackageInfoRef ref(Package());
		PackageState state = ref->State();
		ref->SetState(PENDING);

		fPackageManager->SetCurrentActionPackage(ref, true);
		fPackageManager->AddProgressListener(this);

		BString packageName;
		if (ref->IsLocalFile())
			packageName = ref->LocalFilePath();
		else
			packageName = ref->Name();

		const char* packageNameString = packageName.String();
		try {
			fPackageManager->Install(&packageNameString, 1);
		} catch (BFatalErrorException& ex) {
			BString errorString;
			errorString.SetToFormat(
				"Fatal error occurred while installing package %s: "
				"%s (%s)\n", packageNameString, ex.Message().String(),
				ex.Details().String());
			BAlert* alert(new(std::nothrow) BAlert(B_TRANSLATE("Fatal error"),
				errorString, B_TRANSLATE("Close"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT));
			if (alert != NULL)
				alert->Go();
			_SetDownloadedPackagesState(NONE);
			ref->SetState(state);
			return ex.Error();
		} catch (BAbortedByUserException& ex) {
			HDINFO("Installation of package %s is aborted by user: %s",
				packageNameString, ex.Message().String());
			_SetDownloadedPackagesState(NONE);
			ref->SetState(state);
			return B_OK;
		} catch (BNothingToDoException& ex) {
			HDINFO("Nothing to do while installing package %s: %s",
				packageNameString, ex.Message().String());
			return B_OK;
		} catch (BException& ex) {
			HDERROR("Exception occurred while installing package %s: %s",
				packageNameString, ex.Message().String());
			_SetDownloadedPackagesState(NONE);
			ref->SetState(state);
			return B_ERROR;
		}

		fPackageManager->RemoveProgressListener(this);

		_SetDownloadedPackagesState(ACTIVATED);

		return B_OK;
	}

	// DownloadProgressListener
	virtual void DownloadProgressChanged(const char* packageName,
		float progress)
	{
		bigtime_t now = system_time();
		if (now - fLastDownloadUpdate > 250000 || progress == 1.0) {
			BString tempName(packageName);
			tempName.Truncate(tempName.FindFirst('-'));
				// strip version suffix off package filename
			PackageInfoRef ref(FindPackageByName(tempName));
			if (ref.Get() != NULL) {
				ref->SetDownloadProgress(progress);
				fLastDownloadUpdate = now;
			}
		}
	}

	virtual void DownloadProgressComplete(const char* packageName)
	{
		BString tempName(packageName);
		tempName.Truncate(tempName.FindFirst('-'));
			// strip version suffix off package filename
		PackageInfoRef ref(FindPackageByName(tempName));
		if (ref.Get() != NULL) {
			ref->SetDownloadProgress(1.0);
			fDownloadedPackages.insert(ref);
		}
	}

	virtual	void ConfirmedChanges(BPackageManager::InstalledRepository&
		repository)
	{
		BPackageManager::InstalledRepository::PackageList& activationList =
			repository.PackagesToActivate();

		BSolverPackage* package = NULL;
		for (int32 i = 0; (package = activationList.ItemAt(i)); i++) {
			PackageInfoRef ref(FindPackageByName(package->Info().Name()));
			if (ref.Get() != NULL)
				ref->SetState(PENDING);
		}
	}

private:
	void _SetDownloadedPackagesState(PackageState state)
	{
		for (PackageInfoSet::iterator it = fDownloadedPackages.begin();
				it != fDownloadedPackages.end(); ++it) {
			(*it)->SetState(state);
		}
	}

private:
	bigtime_t fLastDownloadUpdate;
	PackageInfoSet fDownloadedPackages;
};


// #pragma mark - UninstallPackageAction


class UninstallPackageAction : public PackageAction,
	private PackageProgressListener {
public:
	UninstallPackageAction(PackageInfoRef package, Model* model)
		:
		PackageAction(PACKAGE_ACTION_UNINSTALL, package, model)
	{
	}

	virtual const char* Label() const
	{
		return B_TRANSLATE("Uninstall");
	}

	virtual status_t Perform()
	{
		fPackageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES);
		PackageInfoRef ref(Package());
		PackageState state = ref->State();
		fPackageManager->SetCurrentActionPackage(ref, false);
		fPackageManager->AddProgressListener(this);
		const char* packageName = ref->Name().String();
		try {
			fPackageManager->Uninstall(&packageName, 1);
		} catch (BFatalErrorException& ex) {
			BString errorString;
			errorString.SetToFormat(
				"Fatal error occurred while uninstalling package %s: "
				"%s (%s)\n", packageName, ex.Message().String(),
				ex.Details().String());
			BAlert* alert(new(std::nothrow) BAlert(B_TRANSLATE("Fatal error"),
				errorString, B_TRANSLATE("Close"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT));
			if (alert != NULL)
				alert->Go();
			ref->SetState(state);
			return ex.Error();
		} catch (BAbortedByUserException& ex) {
			return B_OK;
		} catch (BNothingToDoException& ex) {
			return B_OK;
		} catch (BException& ex) {
			HDERROR("Exception occurred while uninstalling package %s: %s",
				packageName, ex.Message().String());
			ref->SetState(state);
			return B_ERROR;
		}

		fPackageManager->RemoveProgressListener(this);

		ref->SetState(NONE);

		return B_OK;
	}

	void StartApplyingChanges(
		BPackageManager::InstalledRepository& repository)
	{
		BPackageManager::InstalledRepository::PackageList& packages
			= repository.PackagesToDeactivate();
		for (int32 i = 0; i < packages.CountItems(); i++) {
			PackageInfoRef ref(FindPackageByName(packages.ItemAt(i)
					->Name()));
			if (ref.Get() != NULL)
				fRemovedPackages.Add(ref);
		}
	}

	void ApplyingChangesDone(
		BPackageManager::InstalledRepository& repository)
	{
		for (int32 i = 0; i < fRemovedPackages.CountItems(); i++)
			fRemovedPackages.ItemAt(i)->SetState(NONE);
	}

private:
	PackageList fRemovedPackages;
};


// #pragma mark - OpenPackageAction


struct DeskbarLink {
	DeskbarLink()
	{
	}

	DeskbarLink(const BString& path, const BString& link)
		:
		path(path),
		link(link)
	{
	}

	DeskbarLink(const DeskbarLink& other)
		:
		path(other.path),
		link(other.link)
	{
	}

	DeskbarLink& operator=(const DeskbarLink& other)
	{
		if (this == &other)
			return *this;
		path = other.path;
		link = other.link;
		return *this;
	}

	bool operator==(const DeskbarLink& other)
	{
		return path == other.path && link == other.link;
	}

	bool operator!=(const DeskbarLink& other)
	{
		return !(*this == other);
	}

	BString	path;
	BString	link;
};


typedef List<DeskbarLink, false> DeskbarLinkList;


class DeskbarLinkFinder : public BPackageContentHandler {
public:
	DeskbarLinkFinder(DeskbarLinkList& foundLinks)
		:
		fDeskbarLinks(foundLinks)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		BString path = MakePath(entry);
		if (path.FindFirst("data/deskbar/menu") == 0
				&& entry->SymlinkPath() != NULL) {
			HDINFO("found deskbar entry: %s -> %s",
				path.String(), entry->SymlinkPath());
			fDeskbarLinks.Add(DeskbarLink(path, entry->SymlinkPath()));
		}
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

	BString MakePath(const BPackageEntry* entry)
	{
		BString path;
		while (entry != NULL) {
			if (!path.IsEmpty())
				path.Prepend('/', 1);
			path.Prepend(entry->Name());
			entry = entry->Parent();
		}
		return path;
	}

private:
	DeskbarLinkList&	fDeskbarLinks;
};


class OpenPackageAction : public PackageAction {
public:
	OpenPackageAction(PackageInfoRef package, Model* model,
		const DeskbarLink& link)
		:
		PackageAction(PACKAGE_ACTION_OPEN, package, model),
		fDeskbarLink(link),
		fLabel(B_TRANSLATE("Open %DeskbarLink%"))
	{
		BString target = fDeskbarLink.link;
		int32 lastPathSeparator = target.FindLast('/');
		if (lastPathSeparator > 0 && lastPathSeparator + 1 < target.Length())
			target.Remove(0, lastPathSeparator + 1);

		fLabel.ReplaceAll("%DeskbarLink%", target);
	}

	virtual const char* Label() const
	{
		return fLabel;
	}

	virtual status_t Perform()
	{
		status_t status;
		BPath path;
		if (fDeskbarLink.link.FindFirst('/') == 0) {
			status = path.SetTo(fDeskbarLink.link);
			HDINFO("trying to launch (absolute link): %s", path.Path());
		} else {
			int32 location = InstallLocation();
			if (location == B_PACKAGE_INSTALLATION_LOCATION_SYSTEM) {
				status = find_directory(B_SYSTEM_DIRECTORY, &path);
				if (status != B_OK)
					return status;
			} else if (location == B_PACKAGE_INSTALLATION_LOCATION_HOME) {
				status = find_directory(B_USER_DIRECTORY, &path);
				if (status != B_OK)
					return status;
			} else {
				return B_ERROR;
			}

			status = path.Append(fDeskbarLink.path);
			if (status == B_OK)
				status = path.GetParent(&path);
			if (status == B_OK) {
				status = path.Append(fDeskbarLink.link, true);
				HDINFO("trying to launch: %s", path.Path());
			}
		}

		entry_ref ref;
		if (status == B_OK)
			status = get_ref_for_path(path.Path(), &ref);

		if (status == B_OK)
			status = be_roster->Launch(&ref);

		return status;
	}

	static bool FindAppToLaunch(const PackageInfoRef& package,
		DeskbarLinkList& foundLinks)
	{
		if (package.Get() == NULL)
			return false;

		int32 installLocation = InstallLocation(package);

		BPath packagePath;
		if (installLocation == B_PACKAGE_INSTALLATION_LOCATION_SYSTEM) {
			if (find_directory(B_SYSTEM_PACKAGES_DIRECTORY, &packagePath)
				!= B_OK) {
				return false;
			}
		} else if (installLocation == B_PACKAGE_INSTALLATION_LOCATION_HOME) {
			if (find_directory(B_USER_PACKAGES_DIRECTORY, &packagePath)
				!= B_OK) {
				return false;
			}
		} else {
			HDINFO("OpenPackageAction::FindAppToLaunch(): "
				"unknown install location");
			return false;
		}

		packagePath.Append(package->FileName());

		BNoErrorOutput errorOutput;
		BPackageReader reader(&errorOutput);

		status_t status = reader.Init(packagePath.Path());
		if (status != B_OK) {
			HDINFO("OpenPackageAction::FindAppToLaunch(): "
				"failed to init BPackageReader(%s): %s",
				packagePath.Path(), strerror(status));
			return false;
		}

		// Scan package contents for Deskbar links
		DeskbarLinkFinder contentHandler(foundLinks);
		status = reader.ParseContent(&contentHandler);
		if (status != B_OK) {
			HDINFO("OpenPackageAction::FindAppToLaunch(): "
				"failed parse package contents (%s): %s",
				packagePath.Path(), strerror(status));
			return false;
		}

		return foundLinks.CountItems() > 0;
	}

private:
	DeskbarLink		fDeskbarLink;
	BString			fLabel;
};


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


PackageActionList
PackageManager::GetPackageActions(PackageInfoRef package, Model* model)
{
	PackageActionList actionList;
	if (package->IsSystemPackage() || package->IsSystemDependency())
		return actionList;

	int32 state = package->State();
	if (state == ACTIVATED || state == INSTALLED) {
		actionList.Add(PackageActionRef(new UninstallPackageAction(
			package, model), true));

		// Add OpenPackageActions for each deskbar link found in the
		// package
		DeskbarLinkList foundLinks;
		if (OpenPackageAction::FindAppToLaunch(package, foundLinks)
			&& foundLinks.CountItems() < 4) {
			for (int32 i = 0; i < foundLinks.CountItems(); i++) {
				const DeskbarLink& link = foundLinks.ItemAtFast(i);
				actionList.Add(PackageActionRef(new OpenPackageAction(
					package, model, link), true));
			}
		}
	} else if (state == NONE || state == UNINSTALLED) {
		actionList.Add(PackageActionRef(new InstallPackageAction(package,
				model),	true));
	}
	// TODO: activation status

	return actionList;
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
