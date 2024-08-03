/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef POPULATE_PKG_USER_RATINGS_FROM_SERVER_PROCESS__H
#define POPULATE_PKG_USER_RATINGS_FROM_SERVER_PROCESS__H


#include "AbstractProcess.h"

#include "Model.h"
#include "PackageInfo.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include <package/RepositoryCache.h>


class PkgDataLoadState;


/*!	This process will invoke API calls onto the HaikuDepotServer system in order
	to extract user rating data about the nominated package.
*/

class PopulatePkgUserRatingsFromServerProcess : public AbstractProcess
{
public:
								PopulatePkgUserRatingsFromServerProcess(PackageInfoRef packageInfo,
									Model *model);
	virtual						~PopulatePkgUserRatingsFromServerProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
	virtual status_t			RunInternal();

private:
			Model*				fModel;
			PackageInfoRef		fPackageInfo;
};

#endif // POPULATE_PKG_USER_RATINGS_FROM_SERVER_PROCESS__H
