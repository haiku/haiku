/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_PACKAGE_PROCESS_H
#define ABSTRACT_PACKAGE_PROCESS_H


#include <Referenceable.h>

#include "AbstractProcess.h"
#include "PackageInfo.h"


class PackageManager;
class Model;


class AbstractPackageProcess : public BReferenceable, public AbstractProcess {
public:
								AbstractPackageProcess(PackageInfoRef package,
									Model* model);
	virtual						~AbstractPackageProcess();

			int32				InstallLocation() const
									{ return fInstallLocation; }

	static	int32				InstallLocation(const PackageInfoRef& package);

protected:
			PackageInfoRef		FindPackageByName(const BString& name);

protected:
			PackageManager*		fPackageManager;
			PackageInfoRef		fPackage;

private:
			Model*				fModel;
			int32				fInstallLocation;
};


typedef BReference<AbstractPackageProcess> PackageProcessRef;


#endif // ABSTRACT_PACKAGE_PROCESS_H
