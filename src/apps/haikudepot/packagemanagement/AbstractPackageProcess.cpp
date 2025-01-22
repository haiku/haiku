/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AbstractPackageProcess.h"

#include <Autolock.h>

#include "Logger.h"
#include "Model.h"
#include "PackageKitUtils.h"
#include "PackageManager.h"
#include "PackageUtils.h"


using namespace BPackageKit;


AbstractPackageProcess::AbstractPackageProcess(const BString& packageName, Model* model)
	:
	fPackageName(packageName),
	fModel(model)
{
}


AbstractPackageProcess::~AbstractPackageProcess()
{
}


int32
AbstractPackageProcess::InstallLocation() const
{
	PackageInfoRef package = FindPackageByName(fPackageName);
	if (package.IsSet())
		return PackageKitUtils::DeriveInstallLocation(package.Get());
	return B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;
}


PackageInfoRef
AbstractPackageProcess::FindPackageByName(const BString& packageName) const
{
	return fModel->PackageForName(packageName);
}


void
AbstractPackageProcess::SetPackageState(const BString& packageName, PackageState state)
{
	PackageInfoRef package = fModel->PackageForName(packageName);

	if (package.IsSet()) {
		PackageLocalInfoRef localInfo
			= PackageLocalInfoBuilder(package->LocalInfo()).WithState(state).BuildRef();

		PackageInfoRef updatedPackage
			= PackageInfoBuilder(package).WithLocalInfo(localInfo).BuildRef();

		fModel->AddPackage(updatedPackage);

	} else {
		HDERROR("setting state, but the package [%s] is not present", packageName.String());
	}
}


void
AbstractPackageProcess::SetPackageDownloadProgress(const BString& packageName, float value)
{
	PackageInfoRef package = fModel->PackageForName(packageName);

	if (package.IsSet()) {
		PackageLocalInfoRef localInfo
			= PackageLocalInfoBuilder(package->LocalInfo()).WithDownloadProgress(value).BuildRef();

		PackageInfoRef updatedPackage
			= PackageInfoBuilder(package).WithLocalInfo(localInfo).BuildRef();

		fModel->AddPackage(updatedPackage);

	} else {
		HDERROR("setting progress, but the package [%s] is not present", packageName.String());
	}
}


void
AbstractPackageProcess::ClearPackageInstallationLocations(const BString& packageName)
{
	PackageInfoRef package = fModel->PackageForName(packageName);

	if (package.IsSet()) {
		PackageLocalInfoRef localInfo
			= PackageLocalInfoBuilder(package->LocalInfo()).ClearInstallationLocations().BuildRef();

		PackageInfoRef updatedPackage
			= PackageInfoBuilder(package).WithLocalInfo(localInfo).BuildRef();

		fModel->AddPackage(updatedPackage);

	} else {
		HDERROR("clearing installation locations, but the package [%s] is not present",
			packageName.String());
	}
}
