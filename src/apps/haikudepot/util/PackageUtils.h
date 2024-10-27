/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_UTILS_H
#define PACKAGE_UTILS_H


#include "PackageInfo.h"


class PackageUtils {
public:
	static	void			TitleOrName(const PackageInfoRef package, BString& title);
	static	void			Title(const PackageInfoRef package, BString& title);
	static	void			Summary(const PackageInfoRef package, BString& summary);
};

#endif // PACKAGE_UTILS_H
