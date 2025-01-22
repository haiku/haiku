/*
 * Copyright 2018-2025, Andrew Lindesay <apl@lindesay.co.nz>.

 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file included code earlier from `MainWindow.cpp` and
 * copyrights have been latterly been carried across in 2021.
 */


#include "PopulatePkgSizesProcess.h"

#include <Catalog.h>

#include "Logger.h"
#include "PackageKitUtils.h"
#include "PackageUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PopulatePkgSizesProcess"


PopulatePkgSizesProcess::PopulatePkgSizesProcess(Model* model)
	:
	fModel(model),
	fProgress(0)
{
}


PopulatePkgSizesProcess::~PopulatePkgSizesProcess()
{
}


const char*
PopulatePkgSizesProcess::Name() const
{
	return "PopulatePkgSizesProcess";
}


const char*
PopulatePkgSizesProcess::Description() const
{
	return B_TRANSLATE("Populating package sizes");
}


float
PopulatePkgSizesProcess::Progress()
{
	return fProgress;
}


status_t
PopulatePkgSizesProcess::RunInternal()
{
	int32 countPkgs;
	int32 countPkgSized = 0;
	int32 countPkgUnsized = 0;

	HDINFO("[%s] will populate size for pkgs without a size", Name());

	const std::vector<PackageInfoRef> packages = _SnapshotOfPackages();
	std::vector<PackageInfoRef> updatedPackages;

	countPkgs = static_cast<int32>(packages.size());

	for (int32 i = 0; i < countPkgs; i++) {
		PackageInfoRef package = packages[i];
		const char* packageName = package->Name().String();

		if (!package.IsSet())
			HDFATAL("package is not set");

		PackageLocalInfoRef localInfo = package->LocalInfo();

		if (_ShouldDeriveSize(localInfo)) {
			off_t derivedSize = _DeriveSize(package);

			if (derivedSize > 0) {
				PackageLocalInfoBuilder localInfoBuilder;

				if (localInfo.IsSet())
					localInfoBuilder = PackageLocalInfoBuilder(localInfo);

				localInfoBuilder.WithSize(derivedSize);

				PackageLocalInfoRef localInfo = localInfoBuilder.BuildRef();

				updatedPackages.push_back(
					PackageInfoBuilder(package).WithLocalInfo(localInfo).BuildRef());

				countPkgSized++;
				HDDEBUG("[%s] did derive a size for package [%s]", Name(), packageName);
			} else {
				countPkgUnsized++;
				HDDEBUG("[%s] unable to derive a size for package [%s]", Name(), packageName);
			}
		}

		_SetProgress(static_cast<float>(i) / static_cast<float>(countPkgs));
	}

	fModel->AddPackagesWithChange(updatedPackages, PKG_CHANGED_LOCAL_INFO);

	_SetProgress(1.0);

	HDINFO("[%s] did populate size for %" B_PRId32 " packages with %" B_PRId32
		   " already having a size and %" B_PRId32 " unable to derive a size",
		Name(), countPkgSized, countPkgs - (countPkgSized + countPkgUnsized), countPkgUnsized);

	return B_OK;
}


const std::vector<PackageInfoRef>
PopulatePkgSizesProcess::_SnapshotOfPackages() const
{
	return fModel->Packages();
}


bool
PopulatePkgSizesProcess::_ShouldDeriveSize(PackageLocalInfoRef localInfo) const
{
	if (!localInfo.IsSet())
		return true;

	if (localInfo->Size() > 0)
		return false;

	PackageState state = localInfo->State();

	return state == ACTIVATED || state == INSTALLED;
}


off_t
PopulatePkgSizesProcess::_DeriveSize(const PackageInfoRef package) const
{
	BPath path;
	if (PackageKitUtils::DeriveLocalFilePath(package, path) == B_OK) {
		BEntry entry(path.Path());
		struct stat s = {};
		if (entry.GetStat(&s) == B_OK)
			return s.st_size;
		else
			HDDEBUG("unable to get the size of local file [%s]", path.Path());
	} else {
		HDDEBUG("unable to get the local file of package [%s]", package->Name().String());
	}
	return 0;
}


void
PopulatePkgSizesProcess::_SetProgress(float value)
{
	if (!_ShouldProcessProgress() && value != 1.0 && (value - fProgress) < 0.1)
		return;
	fProgress = value;
	_NotifyChanged();
}
