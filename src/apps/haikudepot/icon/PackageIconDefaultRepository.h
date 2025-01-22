/*
 * Copyright 2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ICON_DEFAULT_REPOSITORY_H
#define PACKAGE_ICON_DEFAULT_REPOSITORY_H


#include <HashMap.h>
#include <HashString.h>


#include "LRUCache.h"
#include "PackageIconRepository.h"


class PackageIconDefaultRepository : public PackageIconRepository {
public:
								PackageIconDefaultRepository();
	virtual						~PackageIconDefaultRepository();

	virtual	status_t			GetIcon(const BString& pkgName, uint32 size,
									BitmapHolderRef& bitmapHolderRef);

private:
			void				_InitDefaultVectorIcon();

private:
			uint8*				fVectorData;
			size_t				fVectorDataSize;
			LRUCache<HashString, BitmapHolderRef>
								fCache;
};


#endif // PACKAGE_ICON_DEFAULT_REPOSITORY_H
