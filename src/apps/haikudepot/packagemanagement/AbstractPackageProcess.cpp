/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 * Copyright 2020-2024, Andrew Lindesay <apl@lindesay.co.nz>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AbstractPackageProcess.h"

#include "Logger.h"
#include "Model.h"
#include "PackageKitUtils.h"
#include "PackageManager.h"
#include "PackageUtils.h"


using namespace BPackageKit;

// #pragma mark - PackageAction


AbstractPackageProcess::AbstractPackageProcess(
		PackageInfoRef package, Model* model)
	:
	fPackage(package),
	fModel(model)
{
	if (package.IsSet())
		fInstallLocation = PackageKitUtils::DeriveInstallLocation(package.Get());
	else
		fInstallLocation = B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;

	// TODO: ideally if the package is installed at multiple locations,
	// the user should be able to pick which one to remove.
	// TODO: allow configuring the installation location
	fPackageManager = new(std::nothrow) PackageManager(
		(BPackageInstallationLocation)fInstallLocation);
}


AbstractPackageProcess::~AbstractPackageProcess()
{
	delete fPackageManager;
}


PackageInfoRef
AbstractPackageProcess::FindPackageByName(const BString& name)
{
	return fModel->PackageForName(name);
}


// TODO; will refactor once the models go immutable.
void
AbstractPackageProcess::SetPackageState(PackageInfoRef& package, PackageState state)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = PackageUtils::NewLocalInfo(package);
		localInfo->SetState(state);
		package->SetLocalInfo(localInfo);
	} else {
		HDERROR("setting state, but the package is not set");
	}
}


// TODO; will refactor once the models go immutable.
void
AbstractPackageProcess::SetPackageDownloadProgress(PackageInfoRef& package, float value)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = PackageUtils::NewLocalInfo(package);
		localInfo->SetDownloadProgress(value);
		package->SetLocalInfo(localInfo);
	} else {
		HDERROR("setting progress, but the package is not set");
	}
}


// TODO; will refactor once the models go immutable.
void
AbstractPackageProcess::ClearPackageInstallationLocations(PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = PackageUtils::NewLocalInfo(package);
		localInfo->ClearInstallationLocations();
		package->SetLocalInfo(localInfo);
	} else {
		HDERROR("clearing installation locations, but the package is not set");
	}
}
