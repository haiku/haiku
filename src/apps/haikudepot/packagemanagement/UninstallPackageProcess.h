/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2011, Ingo Weinhold, <ingo_weinhold@gmx.de>
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file has been re-factored from `PackageManager.h` and
 * copyrights have been carried across in 2021.
 */
#ifndef UNINSTALL_PACKAGE_PROCESS_H
#define UNINSTALL_PACKAGE_PROCESS_H


#include "AbstractPackageProcess.h"
#include "PackageProgressListener.h"


class UninstallPackageProcess
	: public AbstractPackageProcess,
		private PackageProgressListener {
public:
								UninstallPackageProcess(
									PackageInfoRef package, Model* model);
	virtual						~UninstallPackageProcess();

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;

			void				StartApplyingChanges(
									BPackageManager::InstalledRepository&
									repository);
			void				ApplyingChangesDone(
									BPackageManager::InstalledRepository&
									repository);

protected:
	virtual	status_t			RunInternal();

private:
			BString				fDescription;
			std::vector<PackageInfoRef>
								fRemovedPackages;
};

#endif // INSTALL_PACKAGE_PROCESS_H
