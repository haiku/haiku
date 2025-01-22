/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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
								PopulatePkgChangelogFromServerProcess(const BString& packageName,
									Model *model);
	virtual						~PopulatePkgChangelogFromServerProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
	virtual status_t			RunInternal();

private:
			status_t			_UpdateChangelog(const BString& value);

private:
			Model*				fModel;
			BString				fPackageName;
};

#endif // POPULATE_PKG_CHANGELOG_FROM_SERVER_PROCESS__H
