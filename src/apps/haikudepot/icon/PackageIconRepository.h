/*
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ICON_REPOSITORY_H
#define PACKAGE_ICON_REPOSITORY_H

#include <String.h>

#include "BitmapHolder.h"
#include "HaikuDepotConstants.h"


class PackageIconRepository : public BReferenceable {
public:
	virtual	status_t			GetIcon(const BString& pkgName, uint32 size,
									BitmapHolderRef& bitmapHolderRef) = 0;
};


typedef BReference<PackageIconRepository> PackageIconRepositoryRef;


#endif // PACKAGE_ICON_REPOSITORY_H
