/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef LOCAL_PKG_DATA_LOAD_PROCESS__H
#define LOCAL_PKG_DATA_LOAD_PROCESS__H


#include "AbstractProcess.h"

#include "Model.h"
#include "PackageInfo.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include <package/RepositoryCache.h>


class PkgDataLoadState;


/*! This process will take the data from the locally stored repositories (HPKR)
    and will extract the packages.  The packages are then loaded into the
    HaikuDepot 'Model'.
*/

class LocalPkgDataLoadProcess : public AbstractProcess {
public:
								LocalPkgDataLoadProcess(
									PackageInfoListener* packageInfoListener,
									Model *model, bool force = false);
	virtual						~LocalPkgDataLoadProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
	virtual status_t			RunInternal();

private:
			void				_NotifyError(const BString& messageText) const;

private:
			Model*				fModel;
			bool				fForce;
			PackageInfoListener*
								fPackageInfoListener;
};

#endif // LOCAL_PKG_DATA_LOAD_PROCESS__H
