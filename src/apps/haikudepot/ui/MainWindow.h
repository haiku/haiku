/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

#include "BulkLoadStateMachine.h"
#include "Model.h"
#include "PackageAction.h"
#include "PackageActionHandler.h"
#include "PackageInfoListener.h"
#include "HaikuDepotConstants.h"


class BCardLayout;
class BMenu;
class BMenuItem;
class BSplitView;
class FeaturedPackagesView;
class FilterView;
class PackageActionsView;
class PackageInfoView;
class PackageListView;
class ScreenshotWindow;
class WorkStatusView;


class MainWindow : public BWindow, private PackageInfoListener,
	private PackageActionHandler {
public:
								MainWindow(const BMessage& settings);
								MainWindow(const BMessage& settings,
									const PackageInfoRef& package);
	virtual						~MainWindow();

	// BWindow interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			void				StoreSettings(BMessage& message) const;

private:
	// PackageInfoListener
	virtual	void				PackageChanged(
									const PackageInfoEvent& event);

private:
	// PackageActionHandler
	virtual	status_t			SchedulePackageActions(
									PackageActionList& list);
	virtual	Model*				GetModel();

private:
			void				_BuildMenu(BMenuBar* menuBar);
			void				_BuildUserMenu(BMenuBar* menuBar);

			void				_RestoreUserName(const BMessage& settings);
			const char*			_WindowFrameName() const;
			void				_RestoreWindowFrame(const BMessage& settings);

			void				_InitWorkerThreads();
			void				_AdoptModel();

			void				_AdoptPackage(const PackageInfoRef& package);
			void				_ClearPackage();

			void				_RefreshRepositories(bool force);
			void				_RefreshPackageList(bool force);

			void				_StartRefreshWorker(bool force = false);
	static	status_t			_RefreshModelThreadWorker(void* arg);
	static	status_t			_PackageActionWorker(void* arg);
	static	status_t			_PopulatePackageWorker(void* arg);
	static	status_t			_PackagesToShowWorker(void* arg);

			void				_NotifyUser(const char* title,
									const char* message);

			void				_OpenLoginWindow(
									const BMessage& onSuccessMessage);
			void				_UpdateAuthorization();
			void				_RatePackage();
			void				_ShowScreenshot();

private:
			FilterView*			fFilterView;
			BCardLayout*		fListLayout;
			FeaturedPackagesView* fFeaturedPackagesView;
			PackageListView*	fPackageListView;
			PackageInfoView*	fPackageInfoView;
			BSplitView*			fSplitView;
			WorkStatusView*		fWorkStatusView;

			ScreenshotWindow*	fScreenshotWindow;

			BMenu*				fUserMenu;
			BMenuItem*			fLogInItem;
			BMenuItem*			fLogOutItem;

			BMenuItem*			fShowFeaturedPackagesItem;
			BMenuItem*			fShowAvailablePackagesItem;
			BMenuItem*			fShowInstalledPackagesItem;
			BMenuItem*			fShowDevelopPackagesItem;
			BMenuItem*			fShowSourcePackagesItem;

			Model				fModel;
			ModelListenerRef	fModelListener;
			PackageList			fVisiblePackages;
			BulkLoadStateMachine
								fBulkLoadStateMachine;

			bool				fTerminating;
			bool				fSinglePackageMode;
			thread_id			fModelWorker;

			thread_id			fPendingActionsWorker;
			PackageActionList	fPendingActions;
			BLocker				fPendingActionsLock;
			sem_id				fPendingActionsSem;

			thread_id			fPopulatePackageWorker;
			PackageInfoRef		fPackageToPopulate;
			BLocker				fPackageToPopulateLock;
			sem_id				fPackageToPopulateSem;

			thread_id			fShowPackagesWorker;
			PackageList			fPackagesToShowList;
			int32				fPackagesToShowListID;
				// atomic, counted up whenever fPackagesToShowList is refilled
			BLocker				fPackagesToShowListLock;
			sem_id				fNewPackagesToShowSem;
			sem_id				fShowPackagesAcknowledgeSem;
};

#endif // MAIN_WINDOW_H
