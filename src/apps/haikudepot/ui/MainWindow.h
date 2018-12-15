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

#include "TabView.h"
#include "Model.h"
#include "PackageAction.h"
#include "PackageActionHandler.h"
#include "PackageInfoListener.h"
#include "ProcessCoordinator.h"
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
	private PackageActionHandler, public ProcessCoordinatorListener {
public:
								MainWindow(const BMessage& settings);
								MainWindow(const BMessage& settings,
									const PackageInfoRef& package);
	virtual						~MainWindow();

	// BWindow interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			void				StoreSettings(BMessage& message) const;

	// ProcessCoordinatorListener
	virtual void				CoordinatorChanged(
									ProcessCoordinatorState& coordinatorState);
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
			void				_BulkLoadProcessCoordinatorFinished(
									ProcessCoordinatorState&
									processCoordinatorState);
			bool				_SelectedPackageHasWebAppRepositoryCode();

			void				_BuildMenu(BMenuBar* menuBar);
			void				_BuildUserMenu(BMenuBar* menuBar);

			void				_RestoreUserName(const BMessage& settings);
			const char*			_WindowFrameName() const;
			void				_RestoreWindowFrame(const BMessage& settings);

			void				_InitWorkerThreads();
			void				_AdoptModel();

			void				_AdoptPackage(const PackageInfoRef& package);
			void				_ClearPackage();

			void				_PopulatePackageAsync(bool forcePopulate);
			void				_StopBulkLoad();
			void				_StartBulkLoad(bool force = false);
			void				_BulkLoadCompleteReceived();

			void				_NotifyWorkStatusChange(const BString& text,
									float progress);
			void				_HandleWorkStatusChangeMessageReceived(
									const BMessage* message);

	static	status_t			_RefreshModelThreadWorker(void* arg);
	static	status_t			_PackageActionWorker(void* arg);
	static	status_t			_PopulatePackageWorker(void* arg);
	static	status_t			_PackagesToShowWorker(void* arg);

			void				_OpenLoginWindow(
									const BMessage& onSuccessMessage);
			void				_UpdateAuthorization();
			void				_UpdateAvailableRepositories();
			void				_RatePackage();
			void				_ShowScreenshot();

private:
			FilterView*			fFilterView;
			TabView*			fListTabs;
			FeaturedPackagesView* fFeaturedPackagesView;
			PackageListView*	fPackageListView;
			PackageInfoView*	fPackageInfoView;
			BSplitView*			fSplitView;
			WorkStatusView*		fWorkStatusView;

			ScreenshotWindow*	fScreenshotWindow;

			BMenu*				fUserMenu;
			BMenu*				fRepositoryMenu;
			BMenuItem*			fLogInItem;
			BMenuItem*			fLogOutItem;

			BMenuItem*			fShowAvailablePackagesItem;
			BMenuItem*			fShowInstalledPackagesItem;
			BMenuItem*			fShowDevelopPackagesItem;
			BMenuItem*			fShowSourcePackagesItem;

			BMenuItem*			fRefreshRepositoriesItem;

			Model				fModel;
			ModelListenerRef	fModelListener;
			PackageList			fVisiblePackages;
			ProcessCoordinator*	fBulkLoadProcessCoordinator;
			BLocker				fBulkLoadProcessCoordinatorLock;

			bool				fSinglePackageMode;

			thread_id			fPendingActionsWorker;
			PackageActionList	fPendingActions;
			BLocker				fPendingActionsLock;
			sem_id				fPendingActionsSem;

			thread_id			fPopulatePackageWorker;
			PackageInfoRef		fPackageToPopulate;
			bool				fForcePopulatePackage;
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
