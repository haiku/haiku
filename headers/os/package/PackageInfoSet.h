/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE_PACKAGE_INFO_SET_H_
#define _PACKAGE_PACKAGE_INFO_SET_H_


#include <SupportDefs.h>


namespace BPackageKit {


class BPackageInfo;


class BPackageInfoSet {
public:
			class Iterator;

public:
								BPackageInfoSet();
								BPackageInfoSet(const BPackageInfoSet& other);
	virtual						~BPackageInfoSet();

			status_t			AddInfo(const BPackageInfo& info);
			void				MakeEmpty();

			uint32				CountInfos() const;
			Iterator			GetIterator() const;

			BPackageInfoSet&	operator=(const BPackageInfoSet& other);

private:
			bool				_CopyOnWrite();

private:
			struct PackageInfo;
			struct PackageInfoHashDefinition;
			struct PackageMap;

			friend class Iterator;

private:
			PackageMap*			fPackageMap;
};


class BPackageInfoSet::Iterator {
public:
								Iterator(const PackageMap* map = NULL);

			bool				HasNext() const;
			const BPackageInfo*	Next();

private:
			friend class BRepositoryCache;

private:
			const PackageMap*	fMap;
			PackageInfo*		fNextInfo;
};


}	// namespace BPackageKit


#endif // _PACKAGE_PACKAGE_INFO_SET_H_
