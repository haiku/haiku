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
#ifndef PACKAGE_PROGRESS_LISTENER_H
#define PACKAGE_PROGRESS_LISTENER_H


#include <package/manager/PackageManager.h>


using BPackageKit::BManager::BPrivate::BPackageManager;


class PackageProgressListener {
	public:
	virtual	~PackageProgressListener();

	virtual	void				DownloadProgressChanged(
									const char* packageName,
									float progress);
	virtual	void				DownloadProgressComplete(
									const char* packageName);

	virtual	void				ConfirmedChanges(
									BPackageManager::InstalledRepository&
										repository);

	virtual	void				StartApplyingChanges(
									BPackageManager::InstalledRepository&
										repository);
	virtual	void				ApplyingChangesDone(
									BPackageManager::InstalledRepository&
										repository);
};


typedef BObjectList<PackageProgressListener> PackageProgressListenerList;


#endif // PACKAGE_PROGRESS_LISTENER_H
