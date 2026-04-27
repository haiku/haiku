/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__CLEAN_UP_ADMIN_DIR_REQUEST_H_
#define _PACKAGE__CLEAN_UP_ADMIN_DIR_REQUEST_H_


#include <String.h>

#include <package/Context.h>
#include <package/Request.h>
#include <package/InstallationLocationInfo.h>


namespace BPackageKit {


class CleanUpAdminDirectoryRequest : public BRequest {
	typedef	BRequest				inherited;

public:
								CleanUpAdminDirectoryRequest(const BContext& context,
									const BInstallationLocationInfo& location,
									time_t cleanupBefore, int32 minStatesToKeep);
	virtual						~CleanUpAdminDirectoryRequest();

			status_t			GetOldStatesCount(size_t& count);
	virtual	status_t			CreateInitialJobs();

private:
	const BInstallationLocationInfo fLocationInfo;
	time_t fCleanupBefore;
	int32 fMinimumStatesToKeep;
};


}	// namespace BPackageKit


#endif // _PACKAGE__CLEAN_UP_ADMIN_DIR_REQUEST_H_
