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
			typedef BPackageInfoSet::Iterator Iterator;

public:
								BRepositoryCache();
	virtual						~BRepositoryCache();

			status_t			SetTo(const BEntry& entry);

			const BRepositoryInfo&	Info() const;
			const BEntry&		Entry() const;
			bool				IsUserSpecific() const;

			void				SetIsUserSpecific(bool isUserSpecific);

			uint32				CountPackages() const;
			Iterator			GetIterator() const;

private:
			struct RepositoryContentHandler;
			struct StandardErrorOutput;

private:
			BEntry				fEntry;
			BRepositoryInfo		fInfo;
			bool				fIsUserSpecific;

			BPackageInfoSet		fPackages;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_CACHE_H_
