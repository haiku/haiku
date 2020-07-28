/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ICON_REPOSITORY_H
#define PACKAGE_ICON_REPOSITORY_H

#include <String.h>

#include "HaikuDepotConstants.h"
#include "SharedBitmap.h"


class PackageIconRepository {
public:
	virtual	bool				HasAnyIcon(const BString& pkgName) = 0;
	virtual	status_t			GetIcon(const BString& pkgName, BitmapSize size,
									BitmapRef& bitmap) = 0;
};


#endif // PACKAGE_ICON_REPOSITORY_H
