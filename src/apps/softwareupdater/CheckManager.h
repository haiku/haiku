/*
 * Copyright 2013-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 *		Brian Hill <supernova@tycho.email>
 */
#ifndef CHECK_MANAGER_H
#define CHECK_MANAGER_H


#include <Bitmap.h>

#include <package/DaemonClient.h>
#include <package/manager/PackageManager.h>

#include "constants.h"
#include "SoftwareUpdaterWindow.h"

class ProblemWindow;

//using namespace BPackageKit;
using BPackageKit::BPackageInstallationLocation;
using BPackageKit::BPrivate::BDaemonClient;
using BPackageKit::BManager::BPrivate::BPackageManager;


class CheckManager : public BPackageManager,
	private BPackageManager::UserInteractionHandler {
public:
								CheckManager(
									BPackageInstallationLocation location,
									bool verbose);

			void				CheckNetworkConnection();
	virtual	void				JobFailed(BSupportKit::BJob* job);
	virtual	void				JobAborted(BSupportKit::BJob* job);
			void				NoUpdatesNotification();

private:
	// UserInteractionHandler
	virtual	void				HandleProblems();
	virtual	void				ConfirmChanges(bool fromMostSpecific);
	virtual	void				Warn(status_t error, const char* format, ...);

	virtual	void				ProgressPackageDownloadStarted(
									const char* packageName);
	virtual	void				ProgressPackageDownloadActive(
									const char* packageName,
									float completionPercentage,
									off_t bytes, off_t totalBytes);
	virtual	void				ProgressPackageDownloadComplete(
									const char* packageName);
	virtual	void				ProgressPackageChecksumStarted(
									const char* packageName);
	virtual	void				ProgressPackageChecksumComplete(
									const char* packageName);

private:
			void				_CountUpdates(InstalledRepository&
									installationRepository,
									int32& updateCount);
			void				_SendNotification(const char* title,
									const char* text);

private:
			BPackageManager::ClientInstallationInterface
									fClientInstallationInterface;
			
			ProblemWindow*			fProblemWindow;
			bool					fVerbose;
			BString					fNotificationId;
			BString					fHeaderChecking;
			BString					fTextContacting;
};


#endif	// CHECK_MANAGER_H 
