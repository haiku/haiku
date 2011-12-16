/*
 * Copyright 2011, Haiku, Inc.
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


class BPackageInfoSet;
class BRepositoryCache;
class BRepositoryConfig;


class BPackageRoster {
public:
								BPackageRoster();
								~BPackageRoster();

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

			status_t			GetActivePackages(
									BPackageInstallationLocation location,
									BPackageInfoSet& packageInfos);

private:
			status_t			_GetRepositoryPath(BPath* path, bool create,
									directory_which whichDir) const;
			status_t			_VisitRepositoryConfigs(const BPath& path,
									BRepositoryConfigVisitor& visitor);
};


}	// namespace BPackageKit


#endif // _PACKAGE__PACKAGE_ROSTER_H_
