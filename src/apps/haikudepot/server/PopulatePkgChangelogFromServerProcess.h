/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.

 */
#ifndef POPULATE_PKG_CHANGELOG_FROM_SERVER_PROCESS__H
#define POPULATE_PKG_CHANGELOG_FROM_SERVER_PROCESS__H


#include "AbstractProcess.h"

#include "Model.h"
#include "PackageInfo.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include <package/RepositoryCache.h>


class PkgDataLoadState;


/*!	This process will take the data from the locally stored repositories (HPKR)
	and will extract the packages.  The packages are then loaded into the
	HaikuDepot 'Model'.
*/

class PopulatePkgChangelogFromServerProcess : public AbstractProcess
{
public:
								PopulatePkgChangelogFromServerProcess(PackageInfoRef packageInfo,
									Model *model);
	virtual						~PopulatePkgChangelogFromServerProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
	virtual status_t			RunInternal();

private:
			Model*				fModel;
			PackageInfoRef		fPackageInfo;
};

#endif // POPULATE_PKG_CHANGELOG_FROM_SERVER_PROCESS__H
