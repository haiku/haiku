/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_UTILS_H
#define PACKAGE_UTILS_H


#include "PackageInfo.h"


class PackageUtils {
public:
	static	void			TitleOrName(const PackageInfoRef& package, BString& title);
	static	void			Title(const PackageInfoRef& package, BString& title);
	static	void			Summary(const PackageInfoRef& package, BString& summary);

	static PackageLocalizedTextRef
							NewLocalizedText(const PackageInfoRef& package);

	static	bool			Viewed(const PackageInfoRef& package);
	static	PackageState	State(const PackageInfoRef& package);
	static	float			DownloadProgress(const PackageInfoRef& package);
	static	bool			IsActivatedOrLocalFile(const PackageInfoRef& package);
	static	off_t			Size(const PackageInfoRef& package);
	static	int32			Flags(const PackageInfoRef& package);

	static	PackageLocalInfoRef
							NewLocalInfo(const PackageInfoRef& package);

	static	const char*		StateToString(PackageState state);

};

#endif // PACKAGE_UTILS_H
