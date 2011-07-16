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
	virtual						~BPackageInfoSet();

			status_t			Init();

			status_t			AddInfo(const BPackageInfo& info);
			void				MakeEmpty();

			uint32				CountInfos() const;
			Iterator			GetIterator() const;

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
								Iterator();
								Iterator(const BPackageInfoSet* set);

			bool				HasNext() const;
			const BPackageInfo*	Next();

private:
			friend class BRepositoryCache;

private:
			const BPackageInfoSet* fSet;
			PackageInfo*		fNextInfo;
};


}	// namespace BPackageKit


#endif // _PACKAGE_PACKAGE_INFO_SET_H_
