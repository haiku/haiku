/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2011, Ingo Weinhold, <ingo_weinhold@gmx.de>
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2021-2026, Andrew Lindesay <apl@lindesay.co.nz>.
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


class InstallPackageProcess : public AbstractPackageProcess, PackageProgressListener {
public:
								InstallPackageProcess(
									const BString& packageName, Model* model);
	virtual						~InstallPackageProcess();

	virtual	const char*			Name() const;
	virtual	const BString		Description();
	virtual float				Progress();

	// DownloadProgressListener
	virtual void 				DownloadProgressChanged(const char* packageName, float progress,
									off_t bytes, off_t totalBytes);
	virtual void				DownloadProgressComplete(const char* packageName);
	virtual	void				ConfirmedChanges(BPackageManager::InstalledRepository& repository);

protected:
	virtual	status_t			RunInternal();

private:
	static	status_t			_DeriveSimplePackageName(const BString& canonicalForm,
									BString& simplePackageName);

			void				_SetInstallingPackagesState(PackageState state,
									bool ignoreMainPackage = false);

			void				_SetDownloadProgress(
									const BString& simplePackageName,
									float progress);

			void				_SetPackageBytes(const BString& packageName, off_t value);

			void				_AddInstallingPackageName(const BString& packageName);
			bool				_InstallingPackagesIsEmpty();
			std::set<BString>	_InstallingPackageNames();

			std::vector<PackageInfoRef>
								_FindPackagesByNames(const std::set<BString>& packageNames) const;

			BString				_DeriveDescription();

			off_t				_DownloadedSize();

	static	float				_DerivedDownloadProgress(PackageInfoRef package);
	static	float				_DerivedAverageDownloadProgress(
									const std::vector<PackageInfoRef>& packages);
	static	off_t				_TotalSizeIfKnown(const std::vector<PackageInfoRef>& packages);

private:
			std::set<BString>	fInstallingPackageNames;
				// Names of the packages that are going to be downloaded and
				// installed. This will also include the main package that the
				// user is attempting to install; fPackageName.

			std::map<BString, off_t>
								fInstallingPackageBytes;
				// This maps the package name to the quantity of bytes that have
				// been downloaded for that package to this point.
};

#endif // INSTALL_PACKAGE_PROCESS_H
