/*
 * Copyright 2022-2025, Andrew Lindesay <apl@lindesay.co.nz>
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
								AbstractPackageProcess(const BString& packageName, Model* model);
	virtual						~AbstractPackageProcess();


protected:
			int32				InstallLocation() const;
			PackageInfoRef		FindPackageByName(const BString& packageName) const;

			void				SetPackageState(const BString& packageName, PackageState state);
			void				SetPackageDownloadProgress(const BString& packageName, float value);
			void				ClearPackageInstallationLocations(const BString& packageName);

protected:
			BString				fPackageName;

private:
			Model*				fModel;
};


typedef BReference<AbstractPackageProcess> PackageProcessRef;


#endif // ABSTRACT_PACKAGE_PROCESS_H
