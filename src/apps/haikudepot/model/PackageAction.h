/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ACTION_H
#define PACKAGE_ACTION_H

#include <Referenceable.h>

#include "PackageInfo.h"


class PackageManager;

enum {
	PACKAGE_ACTION_INSTALL = 0,
	PACKAGE_ACTION_UNINSTALL,
	PACKAGE_ACTION_OPEN,
	PACKAGE_ACTION_MAX
};


class Model;


class PackageAction : public BReferenceable {
public:
								PackageAction(int32 type,
									PackageInfoRef package, Model* model);
	virtual						~PackageAction();

			int32				Type() const
									{ return fType; }

			Model*				GetModel() const
									{ return fModel; }

	virtual const char*			Label() const = 0;

	virtual status_t			Perform() = 0;

			PackageInfoRef		Package() const
									{ return fPackage; }

			int32				InstallLocation() const
									{ return fInstallLocation; }

	static	int32				InstallLocation(const PackageInfoRef& package);

protected:
			PackageInfoRef		FindPackageByName(const BString& name);

protected:
			PackageManager*		fPackageManager;

private:
			PackageInfoRef		fPackage;
			int32				fType;
			Model*				fModel;
			int32				fInstallLocation;
};


typedef BReference<PackageAction> PackageActionRef;
typedef List<PackageActionRef, false> PackageActionList;


#endif // PACKAGE_ACTION_H
