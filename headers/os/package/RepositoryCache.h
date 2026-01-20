/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REPOSITORY_CACHE_H_
#define _PACKAGE__REPOSITORY_CACHE_H_


#include <Entry.h>
#include <String.h>

#include <package/PackageInfoSet.h>
#include <package/RepositoryInfo.h>


namespace BPackageKit {


class BRepositoryCache {
public:
			typedef bool (*GetPackageInfosCallback)(void* /* context */, const BPackageInfo& info);

public:
								BRepositoryCache();
	virtual						~BRepositoryCache();

			status_t			SetTo(const BEntry& entry);

			const BRepositoryInfo&	Info() const;
			const BEntry&		Entry() const;
			bool				IsUserSpecific() const;

			status_t			GetPackageInfos(GetPackageInfosCallback callback, void* context) const;

private:
			struct RepositoryContentHandler;

			status_t			_ReadCache(const BPath& repositoryCachePath,
									BRepositoryInfo& repositoryInfo,
									GetPackageInfosCallback callback, void* context) const;

private:
			BEntry				fEntry;
			BRepositoryInfo		fInfo;
			bool				fIsUserSpecific;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_CACHE_H_
