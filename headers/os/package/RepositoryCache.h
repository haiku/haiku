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


class BPackageInfo;


class BRepositoryCache {
public:
			class Iterator;

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
			struct PackageInfo;
			struct PackageInfoHashDefinition;
			struct PackageMap;
			struct RepositoryContentHandler;
			struct StandardErrorOutput;

			friend class Iterator;

private:
			BEntry				fEntry;
			BRepositoryInfo		fInfo;
			bool				fIsUserSpecific;

			PackageMap*			fPackageMap;
};


class BRepositoryCache::Iterator {
public:
								Iterator();

			bool				HasNext() const;
			const BPackageInfo*	Next();

private:
								Iterator(const BRepositoryCache* cache);

private:
			friend class BRepositoryCache;

private:
			const BRepositoryCache* fCache;
			PackageInfo*		fNextInfo;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_CACHE_H_
