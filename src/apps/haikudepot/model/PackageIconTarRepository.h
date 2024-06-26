/*
 * Copyright 2020-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ICON_TAR_REPOSITORY_H
#define PACKAGE_ICON_TAR_REPOSITORY_H


#include <DataIO.h>
#include <HashMap.h>
#include <HashString.h>
#include <Locker.h>
#include <Path.h>
#include <Referenceable.h>

#include "IconTarPtr.h"
#include "LRUCache.h"
#include "PackageIconRepository.h"


typedef BReference<IconTarPtr> IconTarPtrRef;


class PackageIconTarRepository : public PackageIconRepository {
public:
								PackageIconTarRepository();
	virtual						~PackageIconTarRepository();

			status_t			Init(BPath& tarPath);

			void				AddIconTarPtr(const BString& pkgName,
									BitmapSize size, off_t offset);
	virtual	status_t			GetIcon(const BString& pkgName, uint32 size,
									BitmapHolderRef& bitmapHolderRef);
	virtual	bool				HasAnyIcon(const BString& pkgName);
	virtual	void				Clear();

private:
			void				_Close();

	const	char*				_ToIconCacheKeySuffix(BitmapSize size);
	const	HashString			_ToIconCacheKey(const BString& pkgName,
									BitmapSize size);

			IconTarPtrRef		_GetOrCreateIconTarPtr(const BString& pkgName);
			IconTarPtrRef		_GetIconTarPtr(const BString& pkgName) const;

			status_t			_CreateIconFromTarOffset(off_t offset,
									BitmapHolderRef& bitmapHolderRef);

			status_t			_GetDefaultIcon(uint32 size, BitmapHolderRef& bitmapHolderRef);

	static	BitmapSize			_BestStoredSize(const IconTarPtrRef iconTarPtrRef,
									int32 desiredSize);

			void				_InitDefaultVectorIcon();

private:
			BLocker				fLock;
			BPositionIO*		fTarIo;
			LRUCache<HashString, BitmapHolderRef>
								fIconCache;
			HashMap<HashString, IconTarPtrRef>
								fIconTarPtrs;

			uint8*				fDefaultIconVectorData;
			size_t				fDefaultIconVectorDataSize;
			LRUCache<HashString, BitmapHolderRef>
								fDefaultIconCache;

			BMallocIO*			fIconDataBuffer;
};

#endif // PACKAGE_ICON_TAR_REPOSITORY_H
