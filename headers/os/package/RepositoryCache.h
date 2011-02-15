/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REPOSITORY_CACHE_H_
#define _PACKAGE__REPOSITORY_CACHE_H_


#include <Entry.h>
#include <String.h>

#include <package/RepositoryInfo.h>


namespace BPackageKit {


class BRepositoryCache {
public:
								BRepositoryCache();
								BRepositoryCache(const BEntry& entry);
	virtual						~BRepositoryCache();

			status_t			SetTo(const BEntry& entry);
			status_t			InitCheck() const;

			const BRepositoryInfo&	Info() const;
			const BEntry&		Entry() const;
			bool				IsUserSpecific() const;

			void				SetIsUserSpecific(bool isUserSpecific);

			uint32				PackageCount() const;

private:
			struct PackageMap;

private:
			status_t			fInitStatus;

			BEntry				fEntry;
			BRepositoryInfo		fInfo;
			bool				fIsUserSpecific;

			PackageMap*			fPackageMap;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_CACHE_H_
