/*
 * Copyright 2018-2024, Andrew Lindesay <apl@lindesay.co.nz>.

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
	fModel(model)
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


status_t
PopulatePkgSizesProcess::RunInternal()
{
	int32 countPkgs = 0;
	int32 countPkgSized = 0;
	int32 countPkgUnsized = 0;

	HDINFO("[%s] will populate size for pkgs without a size", Name());

	for (int32 d = 0; d < fModel->CountDepots() && !WasStopped(); d++) {
		DepotInfoRef depotInfo = fModel->DepotAtIndex(d);
		countPkgs += depotInfo->CountPackages();

		for (int32 p = 0; p < depotInfo->CountPackages(); p++) {
			PackageInfoRef packageInfo = depotInfo->PackageAtIndex(p);

			if (!packageInfo.IsSet())
				HDFATAL("package is not set");

			PackageLocalInfoRef localInfo = PackageUtils::NewLocalInfo(packageInfo);
			PackageState state = localInfo->State();

			if (localInfo->Size() <= 0 && (state == ACTIVATED || state == INSTALLED)) {
				off_t derivedSize = _DeriveSize(packageInfo);

				if (derivedSize > 0) {
					localInfo->SetSize(derivedSize);
					countPkgSized++;
					HDDEBUG("[%s] did derive a size for package [%s]", Name(),
						packageInfo->Name().String());
				} else {
					countPkgUnsized++;
					HDDEBUG("[%s] unable to derive a size for package [%s]", Name(),
						packageInfo->Name().String());
				}
			}

			packageInfo->SetLocalInfo(localInfo);
		}
	}

	HDINFO("[%s] did populate size for %" B_PRId32 " packages with %" B_PRId32
		" already having a size and %" B_PRId32 " unable to derive a size",
		Name(), countPkgSized, countPkgs - countPkgSized, countPkgUnsized);

	return B_OK;
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