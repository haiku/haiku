/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
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


class BSplitView;
class FilterView;
class PackageActionsView;
class PackageInfoView;
class PackageListView;


enum {
	MSG_MAIN_WINDOW_CLOSED		= 'mwcl',
};


class MainWindow : public BWindow, private PackageInfoListener,
	private PackageActionHandler {
public:
								MainWindow(BRect frame,
									const BMessage& settings);
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
			void				_AdoptModel();

			void				_AdoptPackage(const PackageInfoRef& package);
			void				_ClearPackage();

			void				_RefreshRepositories(bool force);
			void				_RefreshPackageList();

			void				_StartRefreshWorker(bool force = false);

	static	status_t			_RefreshModelThreadWorker(void* arg);

	static	status_t			_PackageActionWorker(void* arg);


			void				_NotifyUser(const char* title,
									const char* message);

private:
			FilterView*			fFilterView;
			PackageListView*	fPackageListView;
			PackageInfoView*	fPackageInfoView;
			BSplitView*			fSplitView;
			BMenuItem*			fShowDevelopPackagesItem;
			BMenuItem*			fShowSourcePackagesItem;

			Model				fModel;
			PackageList			fVisiblePackages;

			bool				fTerminating;
			thread_id			fModelWorker;

			thread_id			fPendingActionsWorker;
			PackageActionList	fPendingActions;
			BLocker				fPendingActionsLock;
			sem_id				fPendingActionsSem;
};

#endif // MAIN_WINDOW_H
