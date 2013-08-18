/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H


#include "PackageInfo.h"


class PackageAction : public BReferenceable {
public:
								PackageAction(const PackageInfo& package);
	virtual						~PackageAction();

	virtual const char*			Label() const = 0;
	
	// TODO: Perform() needs to be passed a progress listener
	// and it needs a mechanism to report and react to errors. The
	// Package Kit supports this stuff already.
	virtual status_t			Perform() = 0;

			const PackageInfo&	Package() const
									{ return fPackage; }

private:
			PackageInfo			fPackage;
};


typedef BReference<PackageAction> PackageActionRef;
typedef List<PackageActionRef, false> PackageActionList;


class PackageManager {
public:
								PackageManager();
	virtual						~PackageManager();

	virtual	PackageState		GetPackageState(const PackageInfo& package);
	virtual	PackageActionList	GetPackageActions(const PackageInfo& package);
};


#endif // PACKAGE_MANAGER_H
