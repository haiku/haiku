/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef LOCAL_REPOSITORY_UPDATE_PROCESS__H
#define LOCAL_REPOSITORY_UPDATE_PROCESS__H


#include "AbstractProcess.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include <package/Context.h>
#include <package/PackageRoster.h>
#include <package/RepositoryCache.h>

#include "Model.h"
#include "PackageInfo.h"


/*! This process is intended to, for each repository configured on the host,
    retrieve an updated set of data for the repository from the remote
    repository site.  This is typically HPKR data copied over to the local
    machine.  From there, a latter process will process this data by using the
    facilities of the Package Kit.
*/

class LocalRepositoryUpdateProcess : public AbstractProcess {
public:
								LocalRepositoryUpdateProcess(
									Model *model, bool force = false);
	virtual						~LocalRepositoryUpdateProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
	virtual status_t			RunInternal();

private:
			bool				_ShouldRunForRepositoryName(
									const BString& repoName,
									BPackageKit::BPackageRoster& roster,
									BPackageKit::BRepositoryCache* cache);

			status_t			_RunForRepositoryName(const BString& repoName,
									BPackageKit::BContext& context,
									BPackageKit::BPackageRoster& roster,
									BPackageKit::BRepositoryCache* cache);

			void				_NotifyError(const BString& error) const;
			void				_NotifyError(const BString& error,
									const BString& details) const;

private:
			Model*				fModel;
			bool				fForce;
};

#endif // LOCAL_REPOSITORY_UPDATE_PROCESS__H
