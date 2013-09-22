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


class PackageAction : public BReferenceable {
public:
								PackageAction(PackageInfoRef package);
	virtual						~PackageAction();

	virtual const char*			Label() const = 0;

	virtual status_t			Perform() = 0;

			PackageInfoRef		Package() const
									{ return fPackage; }

protected:
			PackageManager*		fPackageManager;

private:
			PackageInfoRef		fPackage;
};


typedef BReference<PackageAction> PackageActionRef;
typedef List<PackageActionRef, false> PackageActionList;


#endif // PACKAGE_ACTION_H
