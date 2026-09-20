/*
 * Copyright 2022-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_KIT_UTILS_H
#define PACKAGE_KIT_UTILS_H


#include <FindDirectory.h>
#include <Path.h>

#include "PackageInfo.h"

#include <package/PackageDefs.h>
#include <package/manager/Exceptions.h>

class PackageKitUtils
{
public:
	static	status_t		DeriveLocalFilePath(const PackageInfoRef package, BPath& result);

	static	BPackageKit::BPackageInstallationLocation
							DeriveInstallLocation(const PackageInfoRef package);

	static	PackageInfoRef	CreatePackageInfo(const BPackageInfo& info);

	static	PackagePublisherInfoRef
							CreatePublisherInfo(const BPackageInfo& info);

	static	BString			ExceptionToAlertString(
								const BPackageKit::BManager::BPrivate::BFatalErrorException* ex);
	static	BString			ExceptionToLogString(
								const BPackageKit::BManager::BPrivate::BFatalErrorException* ex);

private:
	static	status_t		_DeriveDirectoryWhich(
								BPackageKit::BPackageInstallationLocation location,
								directory_which* which);

};


#endif // PACKAGE_KIT_UTILS_H
