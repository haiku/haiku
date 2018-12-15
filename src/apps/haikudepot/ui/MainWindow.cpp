/*
 * Copyright 2015, Axel Dörfler, <axeld@pinc-software.de>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "MainWindow.h"

#include <map>
#include <vector>

#include <stdio.h>
#include <Alert.h>
#include <Autolock.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <CardLayout.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringList.h>
#include <StringView.h>
#include <TabView.h>

#include "AppUtils.h"
#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "DecisionProvider.h"
#include "FeaturedPackagesView.h"
#include "FilterView.h"
#include "Logger.h"
#include "PackageInfoView.h"
#include "PackageListView.h"
#include "PackageManager.h"
#include "ProcessCoordinator.h"
#include "ProcessCoordinatorFactory.h"
#include "RatePackageWindow.h"
#include "support.h"
#include "ScreenshotWindow.h"
#include "UserLoginWindow.h"
#include "WorkStatusView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_BULK_LOAD_DONE		= 'mmwd',
	MSG_REFRESH_REPOS			= 'mrrp',
	MSG_MANAGE_REPOS			= 'mmrp',
	MSG_SOFTWARE_UPDATER		= 'mswu',
	MSG_LOG_IN					= 'lgin',
	MSG_LOG_OUT					= 'lgot',
	MSG_AUTHORIZATION_CHANGED	= 'athc',
	MSG_PACKAGE_CHANGED			= 'pchd',
	MSG_WORK_STATUS_CHANGE		= 'wsch',
	MSG_WORK_STATUS_CLEAR		= 'wscl',

	MSG_SHOW_FEATURED_PACKAGES	= 'sofp',
	MSG_SHOW_AVAILABLE_PACKAGES	= 'savl',
	MSG_SHOW_INSTALLED_PACKAGES	= 'sins',
	MSG_SHOW_SOURCE_PACKAGES	= 'ssrc',
	MSG_SHOW_DEVELOP_PACKAGES	= 'sdvl'
};


using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;


typedef std::map<BString, PackageInfoRef> PackageInfoMap;


struct RefreshWorkerParameters {
	MainWindow* window;
	bool forceRefresh;

	RefreshWorkerParameters(MainWindow* window, bool forceRefresh)
		:
		window(window),
		forceRefresh(forceRefresh)
	{
	}
};


class MessageModelListener : public ModelListener {
public:
	MessageModelListener(const BMessenger& messenger)
		:
		fMessenger(messenger)
	{
	}

	virtual void AuthorizationChanged()
	{
		if (fMessenger.IsValid())
			fMessenger.SendMessage(MSG_AUTHORIZATION_CHANGED);
	}

private:
	BMessenger	fMessenger;
};


MainWindow::MainWindow(const BMessage& settings)
	:
	BWindow(BRect(50, 50, 650, 550), B_TRANSLATE_SYSTEM_NAME("HaikuDepot"),
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fScreenshotWindow(NULL),
	fUserMenu(NULL),
	fLogInItem(NULL),
	fLogOutItem(NULL),
	fModelListener(new MessageModelListener(BMessenger(this)), true),
	fBulkLoadProcessCoordinator(NULL),
	fSinglePackageMode(false)
{
	BMenuBar* menuBar = new BMenuBar("Main Menu");
	_BuildMenu(menuBar);

	BMenuBar* userMenuBar = new BMenuBar("User Menu");
	_BuildUserMenu(userMenuBar);
	set_small_font(userMenuBar);
	userMenuBar->SetExplicitMaxSize(BSize(B_SIZE_UNSET,
		menuBar->MaxSize().height));

	fFilterView = new FilterView();
	fFeaturedPackagesView = new FeaturedPackagesView();
	fPackageListView = new PackageListView(fModel.Lock());
	fPackageInfoView = new PackageInfoView(fModel.Lock(), this);

	fSplitView = new BSplitView(B_VERTICAL, 5.0f);

	fWorkStatusView = new WorkStatusView("work status");
	fPackageListView->AttachWorkStatusView(fWorkStatusView);

	fListTabs = new TabView(BMessenger(this),
		BMessage(MSG_SHOW_FEATURED_PACKAGES), "list tabs");
	fListTabs->AddTab(fFeaturedPackagesView);
	fListTabs->AddTab(fPackageListView);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(menuBar, 1.0f)
			.Add(userMenuBar, 0.0f)
		.End()
		.Add(fFilterView)
		.AddSplit(fSplitView)
			.AddGroup(B_VERTICAL)
				.Add(fListTabs)
				.SetInsets(
					B_USE_DEFAULT_SPACING, 0.0f,
					B_USE_DEFAULT_SPACING, 0.0f)
			.End()
			.Add(fPackageInfoView)
		.End()
		.Add(fWorkStatusView)
	;

	fSplitView->SetCollapsible(0, false);
	fSplitView->SetCollapsible(1, false);

	fModel.AddListener(fModelListener);

	// Restore settings
	BMessage columnSettings;
	if (settings.FindMessage("column settings", &columnSettings) == B_OK)
		fPackageListView->LoadState(&columnSettings);

	bool showOption;
	if (settings.FindBool("show featured packages", &showOption) == B_OK)
		fModel.SetShowFeaturedPackages(showOption);
	if (settings.FindBool("show available packages", &showOption) == B_OK)
		fModel.SetShowAvailablePackages(showOption);
	if (settings.FindBool("show installed packages", &showOption) == B_OK)
		fModel.SetShowInstalledPackages(showOption);
	if (settings.FindBool("show develop packages", &showOption) == B_OK)
		fModel.SetShowDevelopPackages(showOption);
	if (settings.FindBool("show source packages", &showOption) == B_OK)
		fModel.SetShowSourcePackages(showOption);

	if (fModel.ShowFeaturedPackages())
		fListTabs->Select(0);
	else
		fListTabs->Select(1);

	_RestoreUserName(settings);
	_RestoreWindowFrame(settings);

	atomic_set(&fPackagesToShowListID, 0);

	// start worker threads
	BPackageRoster().StartWatching(this,
		B_WATCH_PACKAGE_INSTALLATION_LOCATIONS);

	_StartBulkLoad();

	_InitWorkerThreads();
}


MainWindow::MainWindow(const BMessage& settings, const PackageInfoRef& package)
	:
	BWindow(BRect(50, 50, 650, 350), B_TRANSLATE_SYSTEM_NAME("HaikuDepot"),
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fWorkStatusView(NULL),
	fScreenshotWindow(NULL),
	fUserMenu(NULL),
	fLogInItem(NULL),
	fLogOutItem(NULL),
	fModelListener(new MessageModelListener(BMessenger(this)), true),
	fBulkLoadProcessCoordinator(NULL),
	fSinglePackageMode(true)
{
	fFilterView = new FilterView();
	fPackageListView = new PackageListView(fModel.Lock());
	fPackageInfoView = new PackageInfoView(fModel.Lock(), this);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fPackageInfoView)
		.SetInsets(0, B_USE_WINDOW_INSETS, 0, 0)
	;

	fModel.AddListener(fModelListener);

	// Restore settings
	_RestoreUserName(settings);
	_RestoreWindowFrame(settings);

	fPackageInfoView->SetPackage(package);

	_InitWorkerThreads();
}


MainWindow::~MainWindow()
{
	BPackageRoster().StopWatching(this);

	delete_sem(fPendingActionsSem);
	if (fPendingActionsWorker >= 0)
		wait_for_thread(fPendingActionsWorker, NULL);

	delete_sem(fPackageToPopulateSem);
	if (fPopulatePackageWorker >= 0)
		wait_for_thread(fPopulatePackageWorker, NULL);

	delete_sem(fNewPackagesToShowSem);
	delete_sem(fShowPackagesAcknowledgeSem);
	if (fShowPackagesWorker >= 0)
		wait_for_thread(fShowPackagesWorker, NULL);

	if (fScreenshotWindow != NULL && fScreenshotWindow->Lock())
		fScreenshotWindow->Quit();
}


bool
MainWindow::QuitRequested()
{
	BMessage settings;
	StoreSettings(settings);

	BMessage message(MSG_MAIN_WINDOW_CLOSED);
	message.AddMessage(KEY_WINDOW_SETTINGS, &settings);

	be_app->PostMessage(&message);

	_StopBulkLoad();

	return true;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_BULK_LOAD_DONE:
			_BulkLoadCompleteReceived();
			break;
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			// TODO: ?
			break;

		case B_PACKAGE_UPDATE:
			// TODO: We should do a more selective update depending on the
			// "event", "location", and "change count" fields!
			_StartBulkLoad(false);
			break;

		case MSG_REFRESH_REPOS:
			_StartBulkLoad(true);
			break;

		case MSG_WORK_STATUS_CHANGE:
			_HandleWorkStatusChangeMessageReceived(message);
			break;

		case MSG_MANAGE_REPOS:
			be_roster->Launch("application/x-vnd.Haiku-Repositories");
			break;

		case MSG_SOFTWARE_UPDATER:
			be_roster->Launch("application/x-vnd.haiku-softwareupdater");
			break;

		case MSG_LOG_IN:
			_OpenLoginWindow(BMessage());
			break;

		case MSG_LOG_OUT:
			fModel.SetUsername("");
			break;

		case MSG_AUTHORIZATION_CHANGED:
			_UpdateAuthorization();
			break;

		case MSG_SHOW_FEATURED_PACKAGES:
			// check to see if we aren't already on the current tab
			if (fListTabs->Selection() ==
					(fModel.ShowFeaturedPackages() ? 0 : 1))
				break;
			{
				BAutolock locker(fModel.Lock());
				fModel.SetShowFeaturedPackages(
					fListTabs->Selection() == 0);
			}
			_AdoptModel();
			break;

		case MSG_SHOW_AVAILABLE_PACKAGES:
			{
				BAutolock locker(fModel.Lock());
				fModel.SetShowAvailablePackages(
					!fModel.ShowAvailablePackages());
			}
			_AdoptModel();
			break;

		case MSG_SHOW_INSTALLED_PACKAGES:
			{
				BAutolock locker(fModel.Lock());
				fModel.SetShowInstalledPackages(
					!fModel.ShowInstalledPackages());
			}
			_AdoptModel();
			break;

		case MSG_SHOW_SOURCE_PACKAGES:
			{
				BAutolock locker(fModel.Lock());
				fModel.SetShowSourcePackages(!fModel.ShowSourcePackages());
			}
			_AdoptModel();
			break;

		case MSG_SHOW_DEVELOP_PACKAGES:
			{
				BAutolock locker(fModel.Lock());
				fModel.SetShowDevelopPackages(!fModel.ShowDevelopPackages());
			}
			_AdoptModel();
			break;

			// this may be triggered by, for example, a user rating being added
			// or having been altered.
		case MSG_SERVER_DATA_CHANGED:
		{
			BString name;
			if (message->FindString("name", &name) == B_OK) {
				BAutolock locker(fModel.Lock());
				if (fPackageInfoView->Package()->Name() == name) {
					_PopulatePackageAsync(true);
				} else {
					if (Logger::IsDebugEnabled()) {
						printf("pkg [%s] is updated on the server, but is "
							"not selected so will not be updated.\n",
							name.String());
					}
				}
			}
        	break;
        }

		case MSG_PACKAGE_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) == B_OK) {
				BAutolock locker(fModel.Lock());
				int count = fVisiblePackages.CountItems();
				for (int i = 0; i < count; i++) {
					const PackageInfoRef& package
						= fVisiblePackages.ItemAtFast(i);
					if (package.Get() != NULL && package->Name() == name) {
						locker.Unlock();
						_AdoptPackage(package);
						break;
					}
				}
			} else {
				_ClearPackage();
			}
			break;
		}

		case MSG_CATEGORY_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				name = "";
			{
				BAutolock locker(fModel.Lock());
				fModel.SetCategory(name);
			}
			_AdoptModel();
			break;
		}

		case MSG_DEPOT_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				name = "";
			{
				BAutolock locker(fModel.Lock());
				fModel.SetDepot(name);
			}
			_AdoptModel();
			_UpdateAvailableRepositories();
			break;
		}

		case MSG_SEARCH_TERMS_MODIFIED:
		{
			// TODO: Do this with a delay!
			BString searchTerms;
			if (message->FindString("search terms", &searchTerms) != B_OK)
				searchTerms = "";
			{
				BAutolock locker(fModel.Lock());
				fModel.SetSearchTerms(searchTerms);
			}
			_AdoptModel();
			break;
		}

		case MSG_PACKAGE_CHANGED:
		{
			PackageInfo* info;
			if (message->FindPointer("package", (void**)&info) == B_OK) {
				PackageInfoRef ref(info, true);
				uint32 changes;
				if (message->FindUInt32("changes", &changes) != B_OK)
					changes = 0;
				if ((changes & PKG_CHANGED_STATE) != 0) {
					BAutolock locker(fModel.Lock());
					fModel.SetPackageState(ref, ref->State());
				}

				// Asynchronous updates to the package information
				// can mean that the package needs to be added or
				// removed to/from the visible packages when the current
				// filter parameters are re-evaluated on this package.
				bool wasVisible = fVisiblePackages.Contains(ref);
				bool isVisible;
				{
					BAutolock locker(fModel.Lock());
					// The package didn't get a chance yet to be in the
					// visible package list
					isVisible = fModel.MatchesFilter(ref);

					// Transfer this single package, otherwise we miss
					// other packages if they appear or disappear along
					// with this one before receive a notification for
					// them.
					if (isVisible) {
						fVisiblePackages.Add(ref);
					} else if (wasVisible)
						fVisiblePackages.Remove(ref);
				}

				if (wasVisible != isVisible) {
					if (!isVisible) {
						fPackageListView->RemovePackage(ref);
						fFeaturedPackagesView->RemovePackage(ref);
					} else {
						fPackageListView->AddPackage(ref);
						if (ref->IsProminent())
							fFeaturedPackagesView->AddPackage(ref);
					}
				}

				if (!fSinglePackageMode && (changes & PKG_CHANGED_STATE) != 0)
					fWorkStatusView->PackageStatusChanged(ref);
			}
			break;
		}

		case MSG_RATE_PACKAGE:
			_RatePackage();
			break;

		case MSG_SHOW_SCREENSHOT:
			_ShowScreenshot();
			break;

		case MSG_PACKAGE_WORKER_BUSY:
		{
			BString reason;
			status_t status = message->FindString("reason", &reason);
			if (status != B_OK)
				break;
			if (!fSinglePackageMode)
				fWorkStatusView->SetBusy(reason);
			break;
		}

		case MSG_PACKAGE_WORKER_IDLE:
			if (!fSinglePackageMode)
				fWorkStatusView->SetIdle();
			break;

		case MSG_ADD_VISIBLE_PACKAGES:
		{
			struct SemaphoreReleaser {
				SemaphoreReleaser(sem_id semaphore)
					:
					fSemaphore(semaphore)
				{ }

				~SemaphoreReleaser() { release_sem(fSemaphore); }

				sem_id fSemaphore;
			};

			// Make sure acknowledge semaphore is released even on error,
			// so the worker thread won't be blocked
			SemaphoreReleaser acknowledger(fShowPackagesAcknowledgeSem);

			int32 numPackages = 0;
			type_code unused;
			status_t status = message->GetInfo("package_ref", &unused,
				&numPackages);
			if (status != B_OK || numPackages == 0)
				break;

			int32 listID = 0;
			status = message->FindInt32("list_id", &listID);
			if (status != B_OK)
				break;
			if (listID != atomic_get(&fPackagesToShowListID)) {
				// list is outdated, ignore
				break;
			}

			for (int i = 0; i < numPackages; i++) {
				PackageInfo* packageRaw = NULL;
				status = message->FindPointer("package_ref", i,
					(void**)&packageRaw);
				if (status != B_OK)
					break;
				PackageInfoRef package(packageRaw, true);

				fPackageListView->AddPackage(package);
				if (package->IsProminent())
					fFeaturedPackagesView->AddPackage(package);
			}
			break;
		}

		case MSG_UPDATE_SELECTED_PACKAGE:
		{
			const PackageInfoRef& selectedPackage = fPackageInfoView->Package();
			fFeaturedPackagesView->SelectPackage(selectedPackage, true);
			fPackageListView->SelectPackage(selectedPackage);

			AutoLocker<BLocker> modelLocker(fModel.Lock());
			if (!fVisiblePackages.Contains(fPackageInfoView->Package()))
				fPackageInfoView->Clear();
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MainWindow::StoreSettings(BMessage& settings) const
{
	settings.AddRect(_WindowFrameName(), Frame());
	if (!fSinglePackageMode) {
		settings.AddRect("window frame", Frame());

		BMessage columnSettings;
		fPackageListView->SaveState(&columnSettings);

		settings.AddMessage("column settings", &columnSettings);

		settings.AddBool("show featured packages",
			fModel.ShowFeaturedPackages());
		settings.AddBool("show available packages",
			fModel.ShowAvailablePackages());
		settings.AddBool("show installed packages",
			fModel.ShowInstalledPackages());
		settings.AddBool("show develop packages", fModel.ShowDevelopPackages());
		settings.AddBool("show source packages", fModel.ShowSourcePackages());
	}

	settings.AddString("username", fModel.Username());
}


void
MainWindow::PackageChanged(const PackageInfoEvent& event)
{
	uint32 watchedChanges = PKG_CHANGED_STATE | PKG_CHANGED_PROMINENCE;
	if ((event.Changes() & watchedChanges) != 0) {
		PackageInfoRef ref(event.Package());
		BMessage message(MSG_PACKAGE_CHANGED);
		message.AddPointer("package", ref.Get());
		message.AddUInt32("changes", event.Changes());
		ref.Detach();
			// reference needs to be released by MessageReceived();
		PostMessage(&message);
	}
}


status_t
MainWindow::SchedulePackageActions(PackageActionList& list)
{
	AutoLocker<BLocker> lock(&fPendingActionsLock);
	for (int32 i = 0; i < list.CountItems(); i++) {
		if (!fPendingActions.Add(list.ItemAtFast(i)))
			return B_NO_MEMORY;
	}

	return release_sem_etc(fPendingActionsSem, list.CountItems(), 0);
}


Model*
MainWindow::GetModel()
{
	return &fModel;
}


void
MainWindow::_BuildMenu(BMenuBar* menuBar)
{
	BMenu* menu = new BMenu(B_TRANSLATE("Tools"));
	fRefreshRepositoriesItem = new BMenuItem(
		B_TRANSLATE("Refresh repositories"), new BMessage(MSG_REFRESH_REPOS));
	menu->AddItem(fRefreshRepositoriesItem);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Manage repositories"
		B_UTF8_ELLIPSIS), new BMessage(MSG_MANAGE_REPOS)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Check for updates"
		B_UTF8_ELLIPSIS), new BMessage(MSG_SOFTWARE_UPDATER)));

	menuBar->AddItem(menu);

	fRepositoryMenu = new BMenu(B_TRANSLATE("Repositories"));
	menuBar->AddItem(fRepositoryMenu);

	menu = new BMenu(B_TRANSLATE("Show"));

	fShowAvailablePackagesItem = new BMenuItem(
		B_TRANSLATE("Available packages"),
		new BMessage(MSG_SHOW_AVAILABLE_PACKAGES));
	menu->AddItem(fShowAvailablePackagesItem);

	fShowInstalledPackagesItem = new BMenuItem(
		B_TRANSLATE("Installed packages"),
		new BMessage(MSG_SHOW_INSTALLED_PACKAGES));
	menu->AddItem(fShowInstalledPackagesItem);

	menu->AddSeparatorItem();

	fShowDevelopPackagesItem = new BMenuItem(
		B_TRANSLATE("Develop packages"),
		new BMessage(MSG_SHOW_DEVELOP_PACKAGES));
	menu->AddItem(fShowDevelopPackagesItem);

	fShowSourcePackagesItem = new BMenuItem(
		B_TRANSLATE("Source packages"),
		new BMessage(MSG_SHOW_SOURCE_PACKAGES));
	menu->AddItem(fShowSourcePackagesItem);

	menuBar->AddItem(menu);
}


void
MainWindow::_BuildUserMenu(BMenuBar* menuBar)
{
	fUserMenu = new BMenu(B_TRANSLATE("Not logged in"));

	fLogInItem = new BMenuItem(B_TRANSLATE("Log in" B_UTF8_ELLIPSIS),
		new BMessage(MSG_LOG_IN));
	fUserMenu->AddItem(fLogInItem);

	fLogOutItem = new BMenuItem(B_TRANSLATE("Log out"),
		new BMessage(MSG_LOG_OUT));
	fUserMenu->AddItem(fLogOutItem);

	menuBar->AddItem(fUserMenu);
}


void
MainWindow::_RestoreUserName(const BMessage& settings)
{
	BString username;
	if (settings.FindString("username", &username) == B_OK
		&& username.Length() > 0) {
		fModel.SetUsername(username);
	}
}


const char*
MainWindow::_WindowFrameName() const
{
	if (fSinglePackageMode)
		return "small window frame";

	return "window frame";
}


void
MainWindow::_RestoreWindowFrame(const BMessage& settings)
{
	BRect frame = Frame();

	BRect windowFrame;
	bool fromSettings = false;
	if (settings.FindRect(_WindowFrameName(), &windowFrame) == B_OK) {
		frame = windowFrame;
		fromSettings = true;
	} else if (!fSinglePackageMode) {
		// Resize to occupy a certain screen size
		BRect screenFrame = BScreen(this).Frame();
		float width = frame.Width();
		float height = frame.Height();
		if (width < screenFrame.Width() * .666f
			&& height < screenFrame.Height() * .666f) {
			frame.bottom = frame.top + screenFrame.Height() * .666f;
			frame.right = frame.left
				+ std::min(screenFrame.Width() * .666f, height * 7 / 5);
		}
	}

	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());

	if (fromSettings)
		MoveOnScreen();
	else
		CenterOnScreen();
}


void
MainWindow::_InitWorkerThreads()
{
	fPendingActionsSem = create_sem(0, "PendingPackageActions");
	if (fPendingActionsSem >= 0) {
		fPendingActionsWorker = spawn_thread(&_PackageActionWorker,
			"Planet Express", B_NORMAL_PRIORITY, this);
		if (fPendingActionsWorker >= 0)
			resume_thread(fPendingActionsWorker);
	} else
		fPendingActionsWorker = -1;

	fPackageToPopulateSem = create_sem(0, "PopulatePackage");
	if (fPackageToPopulateSem >= 0) {
		fPopulatePackageWorker = spawn_thread(&_PopulatePackageWorker,
			"Package Populator", B_NORMAL_PRIORITY, this);
		if (fPopulatePackageWorker >= 0)
			resume_thread(fPopulatePackageWorker);
	} else
		fPopulatePackageWorker = -1;

	fNewPackagesToShowSem = create_sem(0, "ShowPackages");
	fShowPackagesAcknowledgeSem = create_sem(0, "ShowPackagesAck");
	if (fNewPackagesToShowSem >= 0 && fShowPackagesAcknowledgeSem >= 0) {
		fShowPackagesWorker = spawn_thread(&_PackagesToShowWorker,
			"Good news everyone", B_NORMAL_PRIORITY, this);
		if (fShowPackagesWorker >= 0)
			resume_thread(fShowPackagesWorker);
	} else
		fShowPackagesWorker = -1;
}


void
MainWindow::_AdoptModel()
{
	fVisiblePackages = fModel.CreatePackageList();

	{
		AutoLocker<BLocker> modelLocker(fModel.Lock());
		AutoLocker<BLocker> listLocker(fPackagesToShowListLock);
		fPackagesToShowList = fVisiblePackages;
		atomic_add(&fPackagesToShowListID, 1);
	}

	fFeaturedPackagesView->Clear();
	fPackageListView->Clear();

	release_sem(fNewPackagesToShowSem);

	BAutolock locker(fModel.Lock());
	fShowAvailablePackagesItem->SetMarked(fModel.ShowAvailablePackages());
	fShowInstalledPackagesItem->SetMarked(fModel.ShowInstalledPackages());
	fShowSourcePackagesItem->SetMarked(fModel.ShowSourcePackages());
	fShowDevelopPackagesItem->SetMarked(fModel.ShowDevelopPackages());

	if (fModel.ShowFeaturedPackages())
		fListTabs->Select(0);
	else
		fListTabs->Select(1);

	fFilterView->AdoptModel(fModel);
}


void
MainWindow::_AdoptPackage(const PackageInfoRef& package)
{
	{
		BAutolock locker(fModel.Lock());
		fPackageInfoView->SetPackage(package);

		if (fFeaturedPackagesView != NULL)
			fFeaturedPackagesView->SelectPackage(package);
		if (fPackageListView != NULL)
			fPackageListView->SelectPackage(package);
	}

	_PopulatePackageAsync(false);
}


void
MainWindow::_ClearPackage()
{
	fPackageInfoView->Clear();
}


void
MainWindow::_StopBulkLoad()
{
	AutoLocker<BLocker> lock(&fBulkLoadProcessCoordinatorLock);

	if (fBulkLoadProcessCoordinator != NULL) {
		printf("will stop full update process coordinator\n");
		fBulkLoadProcessCoordinator->Stop();
	}
}


void
MainWindow::_StartBulkLoad(bool force)
{
	AutoLocker<BLocker> lock(&fBulkLoadProcessCoordinatorLock);

	if (fBulkLoadProcessCoordinator == NULL) {
		fBulkLoadProcessCoordinator
			= ProcessCoordinatorFactory::CreateBulkLoadCoordinator(
				this,
					// PackageInfoListener
				this,
					// ProcessCoordinatorListener
				&fModel, force);
		fBulkLoadProcessCoordinator->Start();
		fRefreshRepositoriesItem->SetEnabled(false);
	}
}


/*! This method is called when there is some change in the bulk load process.
    A change may mean that a new process has started / stopped etc... or it
    may mean that the entire coordinator has finished.
*/

void
MainWindow::CoordinatorChanged(ProcessCoordinatorState& coordinatorState)
{
	AutoLocker<BLocker> lock(&fBulkLoadProcessCoordinatorLock);

	if (fBulkLoadProcessCoordinator == coordinatorState.Coordinator()) {
		if (!coordinatorState.IsRunning())
			_BulkLoadProcessCoordinatorFinished(coordinatorState);
		else {
			_NotifyWorkStatusChange(coordinatorState.Message(),
				coordinatorState.Progress());
				// show the progress to the user.
		}
	} else {
		if (Logger::IsInfoEnabled()) {
			printf("unknown process coordinator changed\n");
		}
	}
}


void
MainWindow::_BulkLoadProcessCoordinatorFinished(
	ProcessCoordinatorState& coordinatorState)
{
	if (coordinatorState.ErrorStatus() != B_OK) {
		AppUtils::NotifySimpleError(
			B_TRANSLATE("Package Update Error"),
			B_TRANSLATE("While updating package data, a problem has arisen "
				"that may cause data to be outdated or missing from the "
				"application's display.  Additional details regarding this "
				"problem may be able to be obtained from the application "
				"logs."));
	}
	BMessenger messenger(this);
	messenger.SendMessage(MSG_BULK_LOAD_DONE);
	// it is safe to delete the coordinator here because it is already known
	// that all of the processes have completed and their threads will have
	// exited safely by this point.
	delete fBulkLoadProcessCoordinator;
	fBulkLoadProcessCoordinator = NULL;
	fRefreshRepositoriesItem->SetEnabled(true);
}


void
MainWindow::_BulkLoadCompleteReceived()
{
	_AdoptModel();
	_UpdateAvailableRepositories();
	fWorkStatusView->SetIdle();
}


/*! Sends off a message to the Window so that it can change the status view
    on the front-end in the UI thread.
*/

void
MainWindow::_NotifyWorkStatusChange(const BString& text, float progress)
{
	BMessage message(MSG_WORK_STATUS_CHANGE);

	if (!text.IsEmpty())
		message.AddString(KEY_WORK_STATUS_TEXT, text);
	message.AddFloat(KEY_WORK_STATUS_PROGRESS, progress);

	this->PostMessage(&message, this);
}


void
MainWindow::_HandleWorkStatusChangeMessageReceived(const BMessage* message)
{
	BString text;
	float progress;

	if (message->FindString(KEY_WORK_STATUS_TEXT, &text) == B_OK)
		fWorkStatusView->SetText(text);

	if (message->FindFloat(KEY_WORK_STATUS_PROGRESS, &progress) == B_OK)
		fWorkStatusView->SetProgress(progress);
}


status_t
MainWindow::_PackageActionWorker(void* arg)
{
	MainWindow* window = reinterpret_cast<MainWindow*>(arg);

	while (acquire_sem(window->fPendingActionsSem) == B_OK) {
		PackageActionRef ref;
		{
			AutoLocker<BLocker> lock(&window->fPendingActionsLock);
			ref = window->fPendingActions.ItemAt(0);
			if (ref.Get() == NULL)
				break;
			window->fPendingActions.Remove(0);
		}

		BMessenger messenger(window);
		BMessage busyMessage(MSG_PACKAGE_WORKER_BUSY);
		BString text(ref->Label());
		text << B_UTF8_ELLIPSIS;
		busyMessage.AddString("reason", text);

		messenger.SendMessage(&busyMessage);
		ref->Perform();
		messenger.SendMessage(MSG_PACKAGE_WORKER_IDLE);
	}

	return 0;
}


/*! This method will cause the package to have its data refreshed from
    the server application.  The refresh happens in the background; this method
    is asynchronous.
*/

void
MainWindow::_PopulatePackageAsync(bool forcePopulate)
{
		// Trigger asynchronous package population from the web-app
	{
		AutoLocker<BLocker> lock(&fPackageToPopulateLock);
		fPackageToPopulate = fPackageInfoView->Package();
		fForcePopulatePackage = forcePopulate;
	}
	release_sem_etc(fPackageToPopulateSem, 1, 0);

	if (Logger::IsDebugEnabled()) {
		printf("pkg [%s] will be updated from the server.\n",
			fPackageToPopulate->Name().String());
	}
}


/*! This method will run in the background.  The thread will block until there
    is a package to be updated.  When the thread unblocks, it will update the
    package with information from the server.
*/

status_t
MainWindow::_PopulatePackageWorker(void* arg)
{
	MainWindow* window = reinterpret_cast<MainWindow*>(arg);

	while (acquire_sem(window->fPackageToPopulateSem) == B_OK) {
		PackageInfoRef package;
		bool force;
		{
			AutoLocker<BLocker> lock(&window->fPackageToPopulateLock);
			package = window->fPackageToPopulate;
			force = window->fForcePopulatePackage;
		}

		if (package.Get() != NULL) {
			uint32 populateFlags = Model::POPULATE_USER_RATINGS
				| Model::POPULATE_SCREEN_SHOTS
				| Model::POPULATE_CHANGELOG;

			if (force)
				populateFlags |= Model::POPULATE_FORCE;

			window->fModel.PopulatePackage(package, populateFlags);

			if (Logger::IsDebugEnabled()) {
				printf("populating package [%s]\n",
					package->Name().String());
			}
		}
	}

	return 0;
}


/* static */ status_t
MainWindow::_PackagesToShowWorker(void* arg)
{
	MainWindow* window = reinterpret_cast<MainWindow*>(arg);

	while (acquire_sem(window->fNewPackagesToShowSem) == B_OK) {
		PackageList packageList;
		int32 listID = 0;
		{
			AutoLocker<BLocker> lock(&window->fPackagesToShowListLock);
			packageList = window->fPackagesToShowList;
			listID = atomic_get(&window->fPackagesToShowListID);
			window->fPackagesToShowList.Clear();
		}

		// Add packages to list views in batches of kPackagesPerUpdate so we
		// don't block the window thread for long with each iteration
		enum {
			kPackagesPerUpdate = 20
		};
		uint32 packagesInMessage = 0;
		BMessage message(MSG_ADD_VISIBLE_PACKAGES);
		BMessenger messenger(window);
		bool listIsOutdated = false;

		for (int i = 0; i < packageList.CountItems(); i++) {
			const PackageInfoRef& package = packageList.ItemAtFast(i);

			if (packagesInMessage >= kPackagesPerUpdate) {
				if (listID != atomic_get(&window->fPackagesToShowListID)) {
					// The model was changed again in the meantime, and thus
					// our package list isn't current anymore. Send no further
					// messags based on this list and go back to start.
					listIsOutdated = true;
					break;
				}

				message.AddInt32("list_id", listID);
				messenger.SendMessage(&message);
				message.MakeEmpty();
				packagesInMessage = 0;

				// Don't spam the window's message queue, which would make it
				// unresponsive (i.e. allows UI messages to get in between our
				// messages). When it has processed the message we just sent,
				// it will let us know by releasing the semaphore.
				acquire_sem(window->fShowPackagesAcknowledgeSem);
			}
			package->AcquireReference();
			message.AddPointer("package_ref", package.Get());
			packagesInMessage++;
		}

		if (listIsOutdated)
			continue;

		// Send remaining package infos, if any, which didn't make it into
		// the last message (count < kPackagesPerUpdate)
		if (packagesInMessage > 0) {
			message.AddInt32("list_id", listID);
			messenger.SendMessage(&message);
			acquire_sem(window->fShowPackagesAcknowledgeSem);
		}

		// Update selected package in list views
		messenger.SendMessage(MSG_UPDATE_SELECTED_PACKAGE);
	}

	return 0;
}


void
MainWindow::_OpenLoginWindow(const BMessage& onSuccessMessage)
{
	UserLoginWindow* window = new UserLoginWindow(this,
		BRect(0, 0, 500, 400), fModel);

	if (onSuccessMessage.what != 0)
		window->SetOnSuccessMessage(BMessenger(this), onSuccessMessage);

	window->Show();
}


void
MainWindow::_UpdateAuthorization()
{
	BString username(fModel.Username());
	bool hasUser = !username.IsEmpty();

	if (fLogOutItem != NULL)
		fLogOutItem->SetEnabled(hasUser);
	if (fLogInItem != NULL) {
		if (hasUser)
			fLogInItem->SetLabel(B_TRANSLATE("Switch account" B_UTF8_ELLIPSIS));
		else
			fLogInItem->SetLabel(B_TRANSLATE("Log in" B_UTF8_ELLIPSIS));
	}

	if (fUserMenu != NULL) {
		BString label;
		if (username.Length() == 0) {
			label = B_TRANSLATE("Not logged in");
		} else {
			label = B_TRANSLATE("Logged in as %User%");
			label.ReplaceAll("%User%", username);
		}
		fUserMenu->Superitem()->SetLabel(label);
	}
}


void
MainWindow::_UpdateAvailableRepositories()
{
	fRepositoryMenu->RemoveItems(0, fRepositoryMenu->CountItems(), true);

	fRepositoryMenu->AddItem(new BMenuItem(B_TRANSLATE("All repositories"),
		new BMessage(MSG_DEPOT_SELECTED)));

	fRepositoryMenu->AddItem(new BSeparatorItem());

	bool foundSelectedDepot = false;
	const DepotList& depots = fModel.Depots();
	for (int i = 0; i < depots.CountItems(); i++) {
		const DepotInfo& depot = depots.ItemAtFast(i);

		if (depot.Name().Length() != 0) {
			BMessage* message = new BMessage(MSG_DEPOT_SELECTED);
			message->AddString("name", depot.Name());
			BMenuItem* item = new BMenuItem(depot.Name(), message);
			fRepositoryMenu->AddItem(item);

			if (depot.Name() == fModel.Depot()) {
				item->SetMarked(true);
				foundSelectedDepot = true;
			}
		}
	}

	if (!foundSelectedDepot)
		fRepositoryMenu->ItemAt(0)->SetMarked(true);
}


bool
MainWindow::_SelectedPackageHasWebAppRepositoryCode()
{
	const PackageInfoRef& package = fPackageInfoView->Package();
	const BString depotName = package->DepotName();

	if (depotName.IsEmpty()) {
		if (Logger::IsDebugEnabled()) {
			printf("the package [%s] has no depot name\n",
				package->Name().String());
		}
	} else {
		const DepotInfo* depot = fModel.DepotForName(depotName);

		if (depot == NULL) {
			printf("the depot [%s] was not able to be found\n",
				depotName.String());
		} else {
			BString repositoryCode = depot->WebAppRepositoryCode();

			if (repositoryCode.IsEmpty()) {
				printf("the depot [%s] has no web app repository code\n",
					depotName.String());
			} else {
				return true;
			}
		}
	}

	return false;
}


void
MainWindow::_RatePackage()
{
	if (!_SelectedPackageHasWebAppRepositoryCode()) {
		BAlert* alert = new(std::nothrow) BAlert(
			B_TRANSLATE("Rating not possible"),
			B_TRANSLATE("This package doesn't seem to be on the HaikuDepot "
				"Server, so it's not possible to create a new rating "
				"or edit an existing rating."),
			B_TRANSLATE("OK"));
		alert->Go();
    	return;
	}

	if (fModel.Username().IsEmpty()) {
		BAlert* alert = new(std::nothrow) BAlert(
			B_TRANSLATE("Not logged in"),
			B_TRANSLATE("You need to be logged into an account before you "
				"can rate packages."),
			B_TRANSLATE("Cancel"),
			B_TRANSLATE("Login or Create account"));

		if (alert == NULL)
			return;

		int32 choice = alert->Go();
		if (choice == 1)
			_OpenLoginWindow(BMessage(MSG_RATE_PACKAGE));
		return;
	}

	// TODO: Allow only one RatePackageWindow
	// TODO: Mechanism for remembering the window frame
	RatePackageWindow* window = new RatePackageWindow(this,
		BRect(0, 0, 500, 400), fModel);
	window->SetPackage(fPackageInfoView->Package());
	window->Show();
}


void
MainWindow::_ShowScreenshot()
{
	// TODO: Mechanism for remembering the window frame
	if (fScreenshotWindow == NULL)
		fScreenshotWindow = new ScreenshotWindow(this, BRect(0, 0, 500, 400));

	if (fScreenshotWindow->LockWithTimeout(1000) != B_OK)
		return;

	fScreenshotWindow->SetPackage(fPackageInfoView->Package());

	if (fScreenshotWindow->IsHidden())
		fScreenshotWindow->Show();
	else
		fScreenshotWindow->Activate();

	fScreenshotWindow->Unlock();
}
