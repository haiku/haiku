/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageAction.h"

#include "PackageManager.h"


using namespace BPackageKit;

// #pragma mark - PackageAction


PackageAction::PackageAction(int32 type, PackageInfoRef package, Model* model)
	:
	fPackage(package),
	fType(type),
	fModel(model)
{
	// TODO: allow configuring the installation location
	fPackageManager = new(std::nothrow) PackageManager(
		B_PACKAGE_INSTALLATION_LOCATION_HOME);
}


PackageAction::~PackageAction()
{
	delete fPackageManager;
}


