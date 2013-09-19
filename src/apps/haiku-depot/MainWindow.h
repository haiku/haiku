/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

#include "Model.h"
#include "PackageManager.h"


class BSplitView;
class FilterView;
class PackageActionsView;
class PackageInfoView;
class PackageListView;

enum {
	MSG_MAIN_WINDOW_CLOSED		= 'mwcl',
};


class MainWindow : public BWindow {
public:
								MainWindow(BRect frame);
	virtual						~MainWindow();

	// BWindow interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_BuildMenu(BMenuBar* menuBar);
			void				_AdoptModel();

			void				_AdoptPackage(const PackageInfoRef& package);
			void				_ClearPackage();

			void				_RefreshRepositories(bool force = false);
			void				_RefreshPackageList();

			void				_StartRefreshWorker();

	static	status_t			_RefreshModelThreadWorker(void* arg);

private:
			FilterView*			fFilterView;
			PackageListView*	fPackageListView;
			PackageInfoView*	fPackageInfoView;
			BSplitView*			fSplitView;

			Model				fModel;
			PackageList			fVisiblePackages;

			PackageManager
								fPackageManager;

			bool				fTerminating;
			thread_id			fModelWorker;
};

#endif // MAIN_WINDOW_H
