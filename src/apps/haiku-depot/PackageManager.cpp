/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageManager.h"

#include <stdio.h>

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageManager"


// #pragma mark - PackageAction


PackageAction::PackageAction(const PackageInfo& package)
	:
	fPackage(package)
{
}


PackageAction::~PackageAction()
{
}


// #pragma mark - InstallPackageAction


class InstallPackageAction : public PackageAction {
public:
	InstallPackageAction(const PackageInfo& package)
		:
		PackageAction(package)
	{
	}

	virtual const char* Label() const
	{
		return B_TRANSLATE("Install");
	}
	
	virtual status_t Perform()
	{
		// TODO: Trigger asynchronous installation of the package
		return B_ERROR;
	}
};


// #pragma mark - PackageManager


PackageManager::PackageManager()
{
}


PackageManager::~PackageManager()
{
}


PackageState
PackageManager::GetPackageState(const PackageInfo& package)
{
	// TODO: Fetch information from the PackageKit
	return NONE;
}


PackageActionList
PackageManager::GetPackageActions(const PackageInfo& package)
{
	PackageActionList actionList;

	// TODO: Actually fetch applicable actions for this package.
	// If the package is installed and active, it can be
	//		* uninstalled
	//		* deactivated
	// If the package is installed and inactive, it can be
	//		* uninstalled
	//		* activated
	// If the package is not installed, it can be
	//		* installed (and it will be automatically activated)
	actionList.Add(PackageActionRef(new InstallPackageAction(package), true));

	return actionList;
}
