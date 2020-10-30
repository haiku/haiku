/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
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
	virtual	status_t			GetIcon(const BString& pkgName, BitmapSize size,
									BitmapRef& bitmap);
	virtual	bool				HasAnyIcon(const BString& pkgName);

	static	void				CleanupDefaultIcon();

private:
			void				_Close();

	const	char*				_ToIconCacheKeySuffix(BitmapSize size);
	const	HashString			_ToIconCacheKey(const BString& pkgName,
									BitmapSize size);

			IconTarPtrRef		_GetOrCreateIconTarPtr(const BString& pkgName);
			IconTarPtrRef		_GetIconTarPtr(const BString& pkgName) const;

			status_t			_CreateIconFromTarOffset(off_t offset,
									BitmapRef& bitmap);

	static	off_t				_OffsetToBestRepresentation(
									const IconTarPtrRef iconTarPtrRef,
									BitmapSize desiredSize,
									BitmapSize* actualSize);

private:
			BLocker				fLock;
			BPositionIO*		fTarIo;
			LRUCache<HashString, BitmapRef>
								fIconCache;
			HashMap<HashString, IconTarPtrRef>
								fIconTarPtrs;

	static	BitmapRef			sDefaultIcon;
};

#endif // PACKAGE_ICON_TAR_REPOSITORY_H
