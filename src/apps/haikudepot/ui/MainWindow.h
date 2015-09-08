/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

#include "Model.h"
#include "PackageAction.h"
#include "PackageActionHandler.h"
#include "PackageInfoListener.h"


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


enum {
	MSG_MAIN_WINDOW_CLOSED		= 'mwcl',
	MSG_PACKAGE_SELECTED		= 'pkgs',
};


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
};

#endif // MAIN_WINDOW_H
