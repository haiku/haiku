/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2011, Ingo Weinhold, <ingo_weinhold@gmx.de>
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file has been re-factored from `PackageManager.h` and
 * copyrights have been carried across in 2021.
 */
#ifndef INSTALL_PACKAGE_PROCESS_H
#define INSTALL_PACKAGE_PROCESS_H

#include <vector>

#include "AbstractPackageProcess.h"
#include "PackageProgressListener.h"


typedef std::set<PackageInfoRef> PackageInfoSet;


class DownloadProgress;


class InstallPackageProcess
	: public AbstractPackageProcess,
		private PackageProgressListener {
public:
								InstallPackageProcess(
									PackageInfoRef package, Model* model);
	virtual						~InstallPackageProcess();

	virtual	const char*			Name() const;
	virtual	const char*			Description() const;
	virtual float				Progress();

	// DownloadProgressListener
	virtual void 				DownloadProgressChanged(
									const char* packageName,
									float progress);
	virtual void				DownloadProgressComplete(
									const char* packageName);
	virtual	void				ConfirmedChanges(
									BPackageManager::InstalledRepository&
									repository);

protected:
	virtual	status_t			RunInternal();

private:
	static	status_t			_DeriveSimplePackageName(
									const BString& canonicalForm,
									BString& simplePackageName);

			void				_SetDownloadedPackagesState(
									PackageState state);

			void				_SetDownloadProgress(
									const BString& simplePackageName,
									float progress);

private:
			std::vector<DownloadProgress>
								fDownloadProgresses;
			BString				fDescription;
			bigtime_t			fLastDownloadUpdate;
			PackageInfoSet		fDownloadedPackages;
};

#endif // INSTALL_PACKAGE_PROCESS_H
