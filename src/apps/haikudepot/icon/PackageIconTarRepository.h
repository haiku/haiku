/*
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ICON_TAR_REPOSITORY_H
#define PACKAGE_ICON_TAR_REPOSITORY_H


#include <DataIO.h>
#include <HashMap.h>
#include <HashString.h>
#include <Path.h>
#include <Referenceable.h>

#include "IconTarPtr.h"
#include "LRUCache.h"
#include "PackageIconDefaultRepository.h"
#include "PackageIconRepository.h"


typedef BReference<IconTarPtr> IconTarPtrRef;


class PackageIconTarRepository : public PackageIconRepository {
public:
								PackageIconTarRepository(BPath& tarPath);
	virtual						~PackageIconTarRepository();

			status_t			Init();

			void				AddIconTarPtr(const BString& pkgName,
									BitmapSize size, off_t offset);
	virtual	status_t			GetIcon(const BString& pkgName, uint32 size,
									BitmapHolderRef& bitmapHolderRef);

private:
			void				_Close();

	const	char*				_ToIconCacheKeyPart(BitmapSize size);
	const	HashString			_ToIconCacheKey(const BString& pkgName, BitmapSize storedSize,
									uint32 size);

			IconTarPtrRef		_GetOrCreateIconTarPtr(const BString& pkgName);
			IconTarPtrRef		_GetIconTarPtr(const BString& pkgName) const;

			status_t			_CreateIconFromTarOffset(off_t offset, BitmapSize bitmapSize,
									uint32 size, BitmapHolderRef& bitmapHolderRef);

	static	BitmapSize			_BestStoredSize(const IconTarPtrRef iconTarPtrRef,
									int32 desiredSize);

private:
			BPath				fTarPath;
			BPositionIO*		fTarIo;
			LRUCache<HashString, BitmapHolderRef>
								fIconCache;
			HashMap<HashString, IconTarPtrRef>
								fIconTarPtrs;

			BMallocIO*			fIconDataBuffer;

			PackageIconDefaultRepository
								fFallbackRepository;
};

#endif // PACKAGE_ICON_TAR_REPOSITORY_H
