/*
 * Copyright 2011-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_ROSTER_H_
#define _PACKAGE__PACKAGE_ROSTER_H_


#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>

#include <package/PackageDefs.h>


class BStringList;


namespace BPackageKit {


struct BRepositoryConfigVisitor {
	virtual ~BRepositoryConfigVisitor()
	{
	}

	virtual status_t operator()(const BEntry& entry) = 0;
};


class BInstallationLocationInfo;
class BPackageInfoSet;
class BRepositoryCache;
class BRepositoryConfig;
class BPackageInfo;


// watchable events
enum {
	B_WATCH_PACKAGE_INSTALLATION_LOCATIONS	= 0x0001,
		// de-/activation of packages in standard installation locations
};

// notification message "event" field values
enum {
	B_INSTALLATION_LOCATION_PACKAGES_CHANGED,
		// "location": int32
		//		the installation location
		//		(B_PACKAGE_INSTALLATION_LOCATION_{SYSTEM,HOME}
		// "change count": int64
		//		the installation location change count
};


class BPackageRoster {
public:
								BPackageRoster();
								~BPackageRoster();

			bool				IsRebootNeeded();

			status_t			GetCommonRepositoryCachePath(BPath* path,
									bool create = false) const;
			status_t			GetUserRepositoryCachePath(BPath* path,
									bool create = false) const;

			status_t			GetCommonRepositoryConfigPath(BPath* path,
									bool create = false) const;
			status_t			GetUserRepositoryConfigPath(BPath* path,
									bool create = false) const;

			status_t			GetRepositoryNames(BStringList& names);

			status_t			VisitCommonRepositoryConfigs(
									BRepositoryConfigVisitor& visitor);
			status_t			VisitUserRepositoryConfigs(
									BRepositoryConfigVisitor& visitor);

			status_t			GetRepositoryCache(const BString& name,
									BRepositoryCache* repositoryCache);
			status_t			GetRepositoryConfig(const BString& name,
									BRepositoryConfig* repositoryConfig);

			status_t			GetInstallationLocationInfo(
									BPackageInstallationLocation location,
									BInstallationLocationInfo& _info);
			status_t			GetActivePackages(
									BPackageInstallationLocation location,
									BPackageInfoSet& packageInfos);

			status_t			IsPackageActive(
									BPackageInstallationLocation location,
									const BPackageInfo info, bool* active);

			status_t			StartWatching(const BMessenger& target,
									uint32 eventMask);
			status_t			StopWatching(const BMessenger& target);

private:
			status_t			_GetRepositoryPath(BPath* path, bool create,
									directory_which whichDir) const;
			status_t			_VisitRepositoryConfigs(const BPath& path,
									BRepositoryConfigVisitor& visitor);
};


}	// namespace BPackageKit


#endif // _PACKAGE__PACKAGE_ROSTER_H_
