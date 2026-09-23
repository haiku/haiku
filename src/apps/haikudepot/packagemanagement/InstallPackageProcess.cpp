/*
 * Copyright 2013-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 * 		Stephan Aßmus <superstippi@gmx.de>
 * 		Rene Gollent <rene@gollent.com>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Andrew Lindesay <apl@lindesay.co.nz>
 *
 * Note that this file has been re-factored from `PackageManager.cpp` and
 * authors have been carried across in 2021.
 */


#include "InstallPackageProcess.h"

#include <algorithm>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Catalog.h>
#include <StringForSize.h>

#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReader.h>
#include <package/manager/Exceptions.h>
#include <package/solver/SolverPackage.h>

#include "AppUtils.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "Model.h"
#include "PackageKitUtils.h"
#include "PackageManager.h"
#include "PackageUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InstallPackageProcess"

using namespace BPackageKit;
using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;

using BPackageKit::BSolver;
using BPackageKit::BSolverPackage;
using BPackageKit::BSolverRepository;
using BPackageKit::BHPKG::BNoErrorOutput;
using BPackageKit::BHPKG::BPackageContentHandler;
using BPackageKit::BHPKG::BPackageEntry;
using BPackageKit::BHPKG::BPackageEntryAttribute;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BPackageReader;


InstallPackageProcess::InstallPackageProcess(const BString& packageName, Model* model)
	:
	AbstractPackageProcess(packageName, model),
	fInstallingPackageNames(),
	fInstallingPackageBytes()
{
}


InstallPackageProcess::~InstallPackageProcess()
{
}


const char*
InstallPackageProcess::Name() const
{
	return "InstallPackageProcess";
}


const BString
InstallPackageProcess::Description()
{
	return _DeriveDescription();
}


float
InstallPackageProcess::Progress()
{
	if (ProcessState() == PROCESS_RUNNING && !_InstallingPackagesIsEmpty()) {
		std::vector<PackageInfoRef> packages = _FindPackagesByNames(_InstallingPackageNames());
		off_t totalSize = _TotalSizeIfKnown(packages);

		// If all packages' sizes are present then do a weighted progress based on the sizes. If
		// not then return a non-weighted result.

		if (totalSize <= 0)
			return _DerivedAverageDownloadProgress(packages);

		return static_cast<float>(_DownloadedSize()) / static_cast<float>(totalSize);
	}
	return kProgressIndeterminate;
}


status_t
InstallPackageProcess::RunInternal()
{
	// TODO: allow configuring the installation location

	PackageManager* packageManager = new(std::nothrow)
		PackageManager(static_cast<BPackageInstallationLocation>(InstallLocation()));
	ObjectDeleter<PackageManager> packageManagerDeleter(packageManager);

	PackageInfoRef package = FindPackageByName(fPackageName);
	PackageState state = PackageUtils::State(package);

	if (state != NONE && state != UNINSTALLED) {
		HDERROR("the package [%s] cannot be installed because it currently has state [%s]",
			fPackageName.String(), PackageUtils::StateToString(state));
		return B_BAD_VALUE;
	}

	SetPackageState(fPackageName, PENDING);

	packageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
		| BPackageManager::B_ADD_REMOTE_REPOSITORIES | BPackageManager::B_REFRESH_REPOSITORIES);

	packageManager->SetCurrentActionPackage(package, true);
	packageManager->AddProgressListener(this);

	BString packageName = fPackageName;
	PackageLocalInfoRef localInfo = package->LocalInfo();

	if (localInfo.IsSet() && localInfo->IsLocalFile())
		packageName = localInfo->LocalFilePath();

	const char* packageNameString = packageName.String();

	try {
		packageManager->Install(&packageNameString, 1);
	} catch (BFatalErrorException& ex) {
		BString logExStr = PackageKitUtils::ExceptionToLogString(&ex);
		HDERROR(logExStr.String());

		AppUtils::NotifySimpleError(SimpleAlert(B_TRANSLATE("Install failure"),
			PackageKitUtils::ExceptionToAlertString(&ex), B_STOP_ALERT));

		_SetInstallingPackagesState(NONE, true);
		SetPackageState(fPackageName, state);

		return ex.Error();
	} catch (BAbortedByUserException& ex) {
		HDINFO("Installation of package %s is aborted by user: %s", packageNameString,
			ex.Message().String());
		_SetInstallingPackagesState(NONE, true);
		SetPackageState(fPackageName, state);
		return B_OK;
	} catch (BNothingToDoException& ex) {
		HDINFO("Nothing to do while installing package %s: %s", packageNameString,
			ex.Message().String());
		_SetInstallingPackagesState(NONE, true);
		SetPackageState(fPackageName, state);
		return B_OK;
	} catch (BException& ex) {
		HDERROR("Exception occurred while installing package %s: %s", packageNameString,
			ex.Message().String());
		_SetInstallingPackagesState(NONE, true);
		SetPackageState(fPackageName, state);
		return B_ERROR;
	}

	packageManager->RemoveProgressListener(this);

	_SetInstallingPackagesState(ACTIVATED);

	return B_OK;
}


// #pragma mark - DownloadProgressListener


void
InstallPackageProcess::DownloadProgressChanged(const char* packageName, float progress, off_t bytes,
	off_t totalBytes)
{

	if (!_ShouldProcessProgress() && progress != 1.0)
		return;

	BString simplePackageName;

	if (_DeriveSimplePackageName(packageName, simplePackageName) != B_OK) {
		HDERROR("malformed canonical package name [%s]", packageName);
		return;
	}

	_SetPackageBytes(simplePackageName, bytes);

	PackageInfoRef package = FindPackageByName(simplePackageName);

	if (package.IsSet()) {
		PackageLocalInfoBuilder localInfoBuilder = PackageLocalInfoBuilder(package->LocalInfo())
													   .WithDownloadProgress(progress)
													   .WithState(DOWNLOADING);

		if (totalBytes > 0)
			localInfoBuilder.WithSize(totalBytes);

		PackageInfoRef updatedPackage
			= PackageInfoBuilder(package).WithLocalInfo(localInfoBuilder.BuildRef()).BuildRef();

		fModel->AddPackage(updatedPackage);

		HDTRACE("package [%s]; write progress %f", simplePackageName.String(), progress);

		_NotifyChanged();
	} else {
		HDERROR("unable to find package [%s]", packageName);
	}
}


void
InstallPackageProcess::DownloadProgressComplete(const char* packageName)
{
	BString simplePackageName;

	if (_DeriveSimplePackageName(packageName, simplePackageName) != B_OK) {
		HDERROR("malformed canonical package name [%s]", packageName);
		return;
	}

	PackageInfoRef package = FindPackageByName(simplePackageName);

	if (package.IsSet()) {
		PackageLocalInfoBuilder localInfoBuilder = PackageLocalInfoBuilder(package->LocalInfo())
													   .WithDownloadProgress(1.0f)
													   .WithState(DOWNLOADING);

		PackageInfoRef updatedPackage
			= PackageInfoBuilder(package).WithLocalInfo(localInfoBuilder.BuildRef()).BuildRef();

		fModel->AddPackage(updatedPackage);

		HDTRACE("package [%s]; write progress completed", simplePackageName.String());

		_NotifyChanged();
	} else {
		HDERROR("unable to find package [%s]", packageName);
	}
}


void
InstallPackageProcess::ConfirmedChanges(BPackageManager::InstalledRepository& repository)
{
	BPackageManager::InstalledRepository::PackageList& activationList
		= repository.PackagesToActivate();

	BSolverPackage* package = NULL;
	for (int32 i = 0; (package = activationList.ItemAt(i)); i++) {
		BString packageName = package->Info().Name();
		SetPackageState(packageName, PENDING);
		_AddInstallingPackageName(packageName);
			// this will also included the main package again in it.
	}

	_NotifyChanged();
}


/*!	Sets the state for all of the packages that are to be installed.
 *	\param ignoreMainPackage can be provided as true to skip setting the state of the main package.
 */
void
InstallPackageProcess::_SetInstallingPackagesState(PackageState state, bool ignoreMainPackage)
{
	std::set<BString> packageNames = _InstallingPackageNames();
	std::set<BString>::const_iterator it;
	for (it = packageNames.begin(); it != packageNames.end(); ++it) {
		BString packageName = *it;

		if (!ignoreMainPackage || packageName != fPackageName) {
			HDTRACE("will set state for installing package [%s] to [%s] state",
				packageName.String(), PackageUtils::StateToString(state));
			SetPackageState(packageName, state);
		} else {
			HDTRACE("ignoring main package [%s] when setting state [%s]", packageName.String(),
				PackageUtils::StateToString(state));
		}
	}
}


/*!	This method will extract the plain package name from the canonical
 */

// TODO (andponlin) should we be using PackageKit mechanisms for this?
/*static*/ status_t
InstallPackageProcess::_DeriveSimplePackageName(const BString& canonicalForm,
	BString& simplePackageName)
{
	int32 hypenIndex = canonicalForm.FindFirst('-');
	if (hypenIndex <= 0)
		return B_BAD_DATA;
	simplePackageName.SetTo(canonicalForm);
	simplePackageName.Truncate(hypenIndex);
	return B_OK;
}


/*static*/ float
InstallPackageProcess::_DerivedAverageDownloadProgress(const std::vector<PackageInfoRef>& packages)
{
	float sum = 0.0f;
	std::vector<PackageInfoRef>::const_iterator it;

	for (it = packages.begin(); it != packages.end(); ++it) {
		const PackageInfoRef package = *it;
		sum += _DerivedDownloadProgress(package);
	}

	return sum / static_cast<float>(packages.size());
}


/*static*/ float
InstallPackageProcess::_DerivedDownloadProgress(PackageInfoRef package)
{
	if (!package.IsSet())
		return 0.0f;

	PackageLocalInfoRef localInfo = package->LocalInfo();

	if (!localInfo.IsSet())
		return 0.0f;

	switch (localInfo->State()) {
		case DOWNLOADING:
			return localInfo->DownloadProgress();
		case INSTALLED:
		case ACTIVATED:
			return 1.0f;
		default:
			return 0.0f;
	}
}


void
InstallPackageProcess::_SetPackageBytes(const BString& packageName, off_t value)
{
	AutoLocker<BLocker> locker(fLock);
	fInstallingPackageBytes[packageName] = value;
}


bool
InstallPackageProcess::_InstallingPackagesIsEmpty()
{
	AutoLocker<BLocker> locker(fLock);
	return fInstallingPackageNames.empty();
}


void
InstallPackageProcess::_AddInstallingPackageName(const BString& packageName)
{
	AutoLocker<BLocker> locker(fLock);
	fInstallingPackageNames.insert(packageName);
}


std::set<BString>
InstallPackageProcess::_InstallingPackageNames()
{
	AutoLocker<BLocker> locker(fLock);
	return fInstallingPackageNames;
		// returns a copy.
}


std::vector<PackageInfoRef>
InstallPackageProcess::_FindPackagesByNames(const std::set<BString>& packageNames) const
{
	std::vector<PackageInfoRef> packages;
	std::set<BString>::const_iterator it;
	for (it = packageNames.begin(); it != packageNames.end(); ++it) {
		BString packageName = *it;
		packages.push_back(FindPackageByName(packageName));
	}
	return packages;
}


/*!	Returns the total size of all the packages. If any of the packages do not
 *	have a size then it will return 0.
 */
/*static*/ off_t
InstallPackageProcess::_TotalSizeIfKnown(const std::vector<PackageInfoRef>& packages)
{
	off_t result = 0;
	std::vector<PackageInfoRef>::const_iterator it;
	for (it = packages.begin(); it != packages.end(); ++it) {
		PackageInfoRef package = *it;
		off_t packageSize = PackageUtils::Size(package);
		if (packageSize <= 0)
			return 0;
		result += packageSize;
	}
	return result;
}


off_t
InstallPackageProcess::_DownloadedSize()
{
	AutoLocker<BLocker> locker(fLock);
	off_t result = 0;
	std::set<BString>::const_iterator it;
	for (it = fInstallingPackageNames.begin(); it != fInstallingPackageNames.end(); ++it) {
		BString packageName = *it;
		if (fInstallingPackageBytes.find(packageName) != fInstallingPackageBytes.end())
			result += fInstallingPackageBytes[packageName];
	}
	return result;
}


BString
InstallPackageProcess::_DeriveDescription()
{
	const std::set<BString> packageNames = _InstallingPackageNames();
	off_t downloadedSize = _DownloadedSize();
	BString result;

	if (downloadedSize > 0) {
		if (packageNames.size() > 1) {
			result = B_TRANSLATE(
				"Downloading and installing \"%MainPackageName%\" + %CountOtherPackages% other(s)");
			result.ReplaceAll("%CountOtherPackages%", BString() << (packageNames.size() - 1));
		} else {
			result = B_TRANSLATE("Downloading and installing \"%MainPackageName%\"");
		}

		char buffer[256];
		string_for_size(downloadedSize, buffer, sizeof(buffer));

		result << " (" << buffer;

		off_t totalSize = _TotalSizeIfKnown(_FindPackagesByNames(packageNames));

		if (totalSize > 0) {
			string_for_size(totalSize, buffer, sizeof(buffer));
			result << " / " << buffer;
		}

		result << ")";

	} else {
		result = B_TRANSLATE("Prepare installation for \"%MainPackageName%\"");
	}

	result.ReplaceAll("%MainPackageName%", fPackageName);

	return result;
}
