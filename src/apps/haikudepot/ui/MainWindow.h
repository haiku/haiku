/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2017-2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

#include <queue>

#include "HaikuDepotConstants.h"
#include "Model.h"
#include "ProcessCoordinator.h"
#include "PackageInfoListener.h"
#include "TabView.h"
#include "UserDetail.h"
#include "UserDetailVerifierProcess.h"


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
class ShuttingDownWindow;
class WorkStatusView;


class MainWindow : public BWindow, private PackageInfoListener,
	private ProcessCoordinatorConsumer, public ProcessCoordinatorListener,
	public UserDetailVerifierListener {
public:
								MainWindow(const BMessage& settings);
								MainWindow(const BMessage& settings,
									const PackageInfoRef& package);
	virtual						~MainWindow();

	// BWindow interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			void				StoreSettings(BMessage& message) const;

	// ProcessCoordinatorConsumer
	virtual	void				Consume(ProcessCoordinator *item);

	// ProcessCoordinatorListener
	virtual void				CoordinatorChanged(
									ProcessCoordinatorState& coordinatorState);

	// UserDetailVerifierProcessListener
	virtual	void				UserCredentialsFailed();
	virtual void				UserUsageConditionsNotLatest(
									const UserDetail& userDetail);
private:
	// PackageInfoListener
	virtual	void				PackageChanged(
									const PackageInfoEvent& event);

private:
			std::vector<DepotInfoRef>
								_CreateSnapshotOfDepots();

			void				_AddProcessCoordinator(
									ProcessCoordinator* item);
			void				_StopProcessCoordinators();
			void				_SpinUntilProcessCoordinatorComplete();

			bool				_SelectedPackageHasWebAppRepositoryCode();

			void				_BuildMenu(BMenuBar* menuBar);
			void				_BuildUserMenu(BMenuBar* menuBar);

			const char*			_WindowFrameName() const;
			void				_RestoreNickname(const BMessage& settings);
			void				_RestoreWindowFrame(const BMessage& settings);
			void				_RestoreModelSettings(const BMessage& settings);

			void				_MaybePromptCanShareAnonymousUserData(
									const BMessage& settings);
			void				_PromptCanShareAnonymousUserData();

			void				_InitWorkerThreads();
			void				_AdoptModelControls();
			void				_AdoptModel();
			void				_AddRemovePackageFromLists(
									const PackageInfoRef& package);

			void				_AdoptPackage(const PackageInfoRef& package);
			void				_ClearPackage();

			void				_IncrementViewCounter(
									const PackageInfoRef& package);

			void				_PopulatePackageAsync(bool forcePopulate);
			void				_StartBulkLoad(bool force = false);
			void				_BulkLoadCompleteReceived(status_t errorStatus);

			void				_NotifyWorkStatusClear();
			void				_HandleWorkStatusClear();

			void				_NotifyWorkStatusChange(const BString& text,
									float progress);
			void				_HandleWorkStatusChangeMessageReceived(
									const BMessage* message);

			void				_HandleExternalPackageUpdateMessageReceived(
									const BMessage* message);

			void				_HandleChangePackageListViewMode();

	static	status_t			_RefreshModelThreadWorker(void* arg);
	static	status_t			_PopulatePackageWorker(void* arg);
	static	status_t			_PackagesToShowWorker(void* arg);

			void				_OpenLoginWindow(
									const BMessage& onSuccessMessage);
			void				_OpenSettingsWindow();
			void				_StartUserVerify();
			void				_UpdateAuthorization();
			void				_UpdateAvailableRepositories();
			void				_RatePackage();
			void				_ShowScreenshot();

			void				_ViewUserUsageConditions(
									UserUsageConditionsSelectionMode mode);

			void				_HandleUserUsageConditionsNotLatest(
									const UserDetail& userDetail);

private:
			FilterView*			fFilterView;
			TabView*			fListTabs;
			FeaturedPackagesView* fFeaturedPackagesView;
			PackageListView*	fPackageListView;
			PackageInfoView*	fPackageInfoView;
			BSplitView*			fSplitView;
			WorkStatusView*		fWorkStatusView;

			ScreenshotWindow*	fScreenshotWindow;
			ShuttingDownWindow*	fShuttingDownWindow;

			BMenu*				fUserMenu;
			BMenu*				fRepositoryMenu;
			BMenuItem*			fLogInItem;
			BMenuItem*			fLogOutItem;
			BMenuItem*			fUsersUserUsageConditionsMenuItem;

			BMenuItem*			fShowAvailablePackagesItem;
			BMenuItem*			fShowInstalledPackagesItem;
			BMenuItem*			fShowDevelopPackagesItem;
			BMenuItem*			fShowSourcePackagesItem;

			BMenuItem*			fRefreshRepositoriesItem;

			Model				fModel;
			ModelListenerRef	fModelListener;

			std::queue<BReference<ProcessCoordinator> >
								fCoordinatorQueue;
			BReference<ProcessCoordinator>
								fCoordinator;
			BLocker				fCoordinatorLock;
			sem_id				fCoordinatorRunningSem;
			bool				fShouldCloseWhenNoProcessesToCoordinate;

			bool				fSinglePackageMode;

			thread_id			fPopulatePackageWorker;
			PackageInfoRef		fPackageToPopulate;
			bool				fForcePopulatePackage;
			BLocker				fPackageToPopulateLock;
			sem_id				fPackageToPopulateSem;
};

#endif // MAIN_WINDOW_H
