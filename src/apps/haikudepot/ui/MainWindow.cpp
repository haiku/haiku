/*
 * Copyright 2015, Axel Dörfler, <axeld@pinc-software.de>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016-2021, Andrew Lindesay <apl@lindesay.co.nz>.
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
#include "SettingsWindow.h"
#include "ShuttingDownWindow.h"
#include "ToLatestUserUsageConditionsWindow.h"
#include "UserLoginWindow.h"
#include "UserUsageConditionsWindow.h"
#include "WorkStatusView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_REFRESH_REPOS						= 'mrrp',
	MSG_MANAGE_REPOS						= 'mmrp',
	MSG_SOFTWARE_UPDATER					= 'mswu',
	MSG_SETTINGS							= 'stgs',
	MSG_LOG_IN								= 'lgin',
	MSG_AUTHORIZATION_CHANGED				= 'athc',
	MSG_CATEGORIES_LIST_CHANGED				= 'clic',
	MSG_PACKAGE_CHANGED						= 'pchd',
	MSG_WORK_STATUS_CHANGE					= 'wsch',
	MSG_WORK_STATUS_CLEAR					= 'wscl',

	MSG_CHANGE_PACKAGE_LIST_VIEW_MODE		= 'cplm',
	MSG_SHOW_AVAILABLE_PACKAGES				= 'savl',
	MSG_SHOW_INSTALLED_PACKAGES				= 'sins',
	MSG_SHOW_SOURCE_PACKAGES				= 'ssrc',
	MSG_SHOW_DEVELOP_PACKAGES				= 'sdvl'
};

#define KEY_ERROR_STATUS				"errorStatus"

#define TAB_PROMINENT_PACKAGES	0
#define TAB_ALL_PACKAGES		1

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


class MainWindowModelListener : public ModelListener {
public:
	MainWindowModelListener(const BMessenger& messenger)
		:
		fMessenger(messenger)
	{
	}

	virtual void AuthorizationChanged()
	{
		if (fMessenger.IsValid())
			fMessenger.SendMessage(MSG_AUTHORIZATION_CHANGED);
	}

	virtual void CategoryListChanged()
	{
		if (fMessenger.IsValid())
			fMessenger.SendMessage(MSG_CATEGORIES_LIST_CHANGED);
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
	fShuttingDownWindow(NULL),
	fUserMenu(NULL),
	fLogInItem(NULL),
	fLogOutItem(NULL),
	fUsersUserUsageConditionsMenuItem(NULL),
	fModelListener(new MainWindowModelListener(BMessenger(this)), true),
	fCoordinator(NULL),
	fShouldCloseWhenNoProcessesToCoordinate(false),
	fSinglePackageMode(false)
{
	if ((fCoordinatorRunningSem = create_sem(1, "ProcessCoordinatorSem")) < B_OK)
		debugger("unable to create the process coordinator semaphore");

	BMenuBar* menuBar = new BMenuBar("Main Menu");
	_BuildMenu(menuBar);

	BMenuBar* userMenuBar = new BMenuBar("User Menu");
	_BuildUserMenu(userMenuBar);
	set_small_font(userMenuBar);
	userMenuBar->SetExplicitMaxSize(BSize(B_SIZE_UNSET,
		menuBar->MaxSize().height));

	fFilterView = new FilterView();
	fFeaturedPackagesView = new FeaturedPackagesView(fModel);
	fPackageListView = new PackageListView(&fModel);
	fPackageInfoView = new PackageInfoView(&fModel, this);

	fSplitView = new BSplitView(B_VERTICAL, 5.0f);

	fWorkStatusView = new WorkStatusView("work status");
	fPackageListView->AttachWorkStatusView(fWorkStatusView);

	fListTabs = new TabView(BMessenger(this),
		BMessage(MSG_CHANGE_PACKAGE_LIST_VIEW_MODE), "list tabs");
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

	BMessage columnSettings;
	if (settings.FindMessage("column settings", &columnSettings) == B_OK)
		fPackageListView->LoadState(&columnSettings);

	_RestoreModelSettings(settings);
	_MaybePromptCanShareAnonymousUserData(settings);

	if (fModel.PackageListViewMode() == PROMINENT)
		fListTabs->Select(TAB_PROMINENT_PACKAGES);
	else
		fListTabs->Select(TAB_ALL_PACKAGES);

	_RestoreNickname(settings);
	_UpdateAuthorization();
	_RestoreWindowFrame(settings);

	// start worker threads
	BPackageRoster().StartWatching(this,
		B_WATCH_PACKAGE_INSTALLATION_LOCATIONS);

	_InitWorkerThreads();
	_AdoptModel();
	_StartBulkLoad();
}


MainWindow::MainWindow(const BMessage& settings, PackageInfoRef& package)
	:
	BWindow(BRect(50, 50, 650, 350), B_TRANSLATE_SYSTEM_NAME("HaikuDepot"),
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fFeaturedPackagesView(NULL),
	fPackageListView(NULL),
	fWorkStatusView(NULL),
	fScreenshotWindow(NULL),
	fShuttingDownWindow(NULL),
	fUserMenu(NULL),
	fLogInItem(NULL),
	fLogOutItem(NULL),
	fUsersUserUsageConditionsMenuItem(NULL),
	fModelListener(new MainWindowModelListener(BMessenger(this)), true),
	fCoordinator(NULL),
	fShouldCloseWhenNoProcessesToCoordinate(false),
	fSinglePackageMode(true)
{
	if ((fCoordinatorRunningSem = create_sem(1, "ProcessCoordinatorSem")) < B_OK)
		debugger("unable to create the process coordinator semaphore");

	fFilterView = new FilterView();
	fPackageInfoView = new PackageInfoView(&fModel, this);
	fWorkStatusView = new WorkStatusView("work status");

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fPackageInfoView)
		.Add(fWorkStatusView)
		.SetInsets(0, B_USE_WINDOW_INSETS, 0, 0)
	;

	fModel.AddListener(fModelListener);

	// add the single package into the model so that any internal
	// business logic is able to find the package.
	DepotInfoRef depot(new DepotInfo("single-pkg-depot"), true);
	depot->AddPackage(package);
	fModel.MergeOrAddDepot(depot);

	// Restore settings
	_RestoreNickname(settings);
	_UpdateAuthorization();
	_RestoreWindowFrame(settings);

	fPackageInfoView->SetPackage(package);

	// start worker threads
	BPackageRoster().StartWatching(this,
		B_WATCH_PACKAGE_INSTALLATION_LOCATIONS);

	_InitWorkerThreads();
}


MainWindow::~MainWindow()
{
	_SpinUntilProcessCoordinatorComplete();
	delete_sem(fCoordinatorRunningSem);
	fCoordinatorRunningSem = 0;

	BPackageRoster().StopWatching(this);

	if (fScreenshotWindow != NULL) {
		if (fScreenshotWindow->Lock())
			fScreenshotWindow->Quit();
	}

	if (fShuttingDownWindow != NULL) {
		if (fShuttingDownWindow->Lock())
			fShuttingDownWindow->Quit();
	}
}


bool
MainWindow::QuitRequested()
{

	_StopProcessCoordinators();

	// If there are any processes in flight we need to be careful to make
	// sure that they are cleanly completed before actually quitting.  By
	// turning on the `fShouldCloseWhenNoProcessesToCoordinate` flag, when
	// the process coordination has completed then it will detect this and
	// quit again.

	{
		AutoLocker<BLocker> lock(&fCoordinatorLock);
		fShouldCloseWhenNoProcessesToCoordinate = true;

		if (fCoordinator.IsSet()) {
			HDINFO("a coordinator is running --> will wait before quitting...");

			if (fShuttingDownWindow == NULL)
				fShuttingDownWindow = new ShuttingDownWindow(this);
			fShuttingDownWindow->Show();

			return false;
		}
	}

	BMessage settings;
	StoreSettings(settings);
	BMessage message(MSG_MAIN_WINDOW_CLOSED);
	message.AddMessage(KEY_WINDOW_SETTINGS, &settings);
	be_app->PostMessage(&message);

	if (fShuttingDownWindow != NULL) {
		if (fShuttingDownWindow->Lock())
			fShuttingDownWindow->Quit();
		fShuttingDownWindow = NULL;
	}

	return true;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_BULK_LOAD_DONE:
		{
			int64 errorStatus64;
			if (message->FindInt64(KEY_ERROR_STATUS, &errorStatus64) == B_OK)
				_BulkLoadCompleteReceived((status_t) errorStatus64);
			else
				HDERROR("expected [%s] value in message", KEY_ERROR_STATUS);
			break;
		}
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			// TODO: ?
			break;

		case B_PACKAGE_UPDATE:
			_HandleExternalPackageUpdateMessageReceived(message);
			break;

		case MSG_REFRESH_REPOS:
			_StartBulkLoad(true);
			break;

		case MSG_WORK_STATUS_CLEAR:
			_HandleWorkStatusClear();
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

		case MSG_SETTINGS:
			_OpenSettingsWindow();
			break;

		case MSG_LOG_OUT:
			fModel.SetNickname("");
			break;

		case MSG_VIEW_LATEST_USER_USAGE_CONDITIONS:
			_ViewUserUsageConditions(LATEST);
			break;

		case MSG_VIEW_USERS_USER_USAGE_CONDITIONS:
			_ViewUserUsageConditions(USER);
			break;

		case MSG_AUTHORIZATION_CHANGED:
			_StartUserVerify();
			_UpdateAuthorization();
			break;

		case MSG_CATEGORIES_LIST_CHANGED:
			fFilterView->AdoptModel(fModel);
			break;

		case MSG_CHANGE_PACKAGE_LIST_VIEW_MODE:
			_HandleChangePackageListViewMode();
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
					HDDEBUG("pkg [%s] is updated on the server, but is "
						"not selected so will not be updated.",
						name.String());
				}
			}
        	break;
        }

		case MSG_PACKAGE_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) == B_OK) {
				PackageInfoRef package;
				{
					BAutolock locker(fModel.Lock());
					package = fModel.PackageForName(name);
				}
				if (!package.IsSet() || name != package->Name())
					debugger("unable to find the named package");
				else {
					_AdoptPackage(package);
					_IncrementViewCounter(package);
				}
			} else {
				_ClearPackage();
			}
			break;
		}

		case MSG_CATEGORY_SELECTED:
		{
			BString code;
			if (message->FindString("code", &code) != B_OK)
				code = "";
			{
				BAutolock locker(fModel.Lock());
				fModel.SetCategory(code);
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
				fFeaturedPackagesView->BeginAddRemove();
				_AddRemovePackageFromLists(ref);
				fFeaturedPackagesView->EndAddRemove();
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
			fWorkStatusView->SetBusy(reason);
			break;
		}

		case MSG_PACKAGE_WORKER_IDLE:
			fWorkStatusView->SetIdle();
			break;

		case MSG_USER_USAGE_CONDITIONS_NOT_LATEST:
		{
			BMessage userDetailMsg;
			if (message->FindMessage("userDetail", &userDetailMsg) != B_OK) {
				debugger("expected the [userDetail] data to be carried in the "
					"message.");
			}
			UserDetail userDetail(&userDetailMsg);
			_HandleUserUsageConditionsNotLatest(userDetail);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


static const char*
main_window_package_list_view_mode_str(package_list_view_mode mode)
{
	if (mode == PROMINENT)
		return "PROMINENT";
	return "ALL";
}


static package_list_view_mode
main_window_str_to_package_list_view_mode(const BString& str)
{
	if (str == "PROMINENT")
		return PROMINENT;
	return ALL;
}


void
MainWindow::StoreSettings(BMessage& settings) const
{
	settings.AddRect(_WindowFrameName(), Frame());
	if (!fSinglePackageMode) {
		settings.AddRect("window frame", Frame());

		BMessage columnSettings;
		if (fPackageListView != NULL)
			fPackageListView->SaveState(&columnSettings);

		settings.AddMessage("column settings", &columnSettings);

		settings.AddString(SETTING_PACKAGE_LIST_VIEW_MODE,
			main_window_package_list_view_mode_str(
				fModel.PackageListViewMode()));
		settings.AddBool(SETTING_SHOW_AVAILABLE_PACKAGES,
			fModel.ShowAvailablePackages());
		settings.AddBool(SETTING_SHOW_INSTALLED_PACKAGES,
			fModel.ShowInstalledPackages());
		settings.AddBool(SETTING_SHOW_DEVELOP_PACKAGES,
			fModel.ShowDevelopPackages());
		settings.AddBool(SETTING_SHOW_SOURCE_PACKAGES,
			fModel.ShowSourcePackages());
		settings.AddBool(SETTING_CAN_SHARE_ANONYMOUS_USER_DATA,
			fModel.CanShareAnonymousUsageData());
	}

	settings.AddString("username", fModel.Nickname());
}


void
MainWindow::Consume(ProcessCoordinator *item)
{
	_AddProcessCoordinator(item);
}


void
MainWindow::PackageChanged(const PackageInfoEvent& event)
{
	uint32 watchedChanges = PKG_CHANGED_STATE | PKG_CHANGED_PROMINENCE;
	if ((event.Changes() & watchedChanges) != 0) {
		PackageInfoRef ref(event.Package());
		BMessage message(MSG_PACKAGE_CHANGED);
		message.AddPointer("package", ref.Get());
		ref.Detach();
			// reference needs to be released by MessageReceived();
		PostMessage(&message);
	}
}


void
MainWindow::_BuildMenu(BMenuBar* menuBar)
{
	BMenu* menu = new BMenu(B_TRANSLATE_SYSTEM_NAME("HaikuDepot"));
	fRefreshRepositoriesItem = new BMenuItem(
		B_TRANSLATE("Refresh repositories"), new BMessage(MSG_REFRESH_REPOS));
	menu->AddItem(fRefreshRepositoriesItem);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Manage repositories"
		B_UTF8_ELLIPSIS), new BMessage(MSG_MANAGE_REPOS)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Check for updates"
		B_UTF8_ELLIPSIS), new BMessage(MSG_SOFTWARE_UPDATER)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(MSG_SETTINGS), ','));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
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

	BMenuItem *latestUserUsageConditionsMenuItem =
		new BMenuItem(B_TRANSLATE("View latest usage conditions"
			B_UTF8_ELLIPSIS),
			new BMessage(MSG_VIEW_LATEST_USER_USAGE_CONDITIONS));
	fUserMenu->AddItem(latestUserUsageConditionsMenuItem);

	fUsersUserUsageConditionsMenuItem =
		new BMenuItem(B_TRANSLATE("View agreed usage conditions"
			B_UTF8_ELLIPSIS),
			new BMessage(MSG_VIEW_USERS_USER_USAGE_CONDITIONS));
	fUserMenu->AddItem(fUsersUserUsageConditionsMenuItem);

	menuBar->AddItem(fUserMenu);
}


void
MainWindow::_RestoreNickname(const BMessage& settings)
{
	BString nickname;
	if (settings.FindString("username", &nickname) == B_OK
		&& nickname.Length() > 0) {
		fModel.SetNickname(nickname);
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
MainWindow::_RestoreModelSettings(const BMessage& settings)
{
	BString packageListViewMode;
	if (settings.FindString(SETTING_PACKAGE_LIST_VIEW_MODE,
			&packageListViewMode) == B_OK) {
		fModel.SetPackageListViewMode(
			main_window_str_to_package_list_view_mode(packageListViewMode));
	}

	bool showOption;
	if (settings.FindBool(SETTING_SHOW_AVAILABLE_PACKAGES, &showOption) == B_OK)
		fModel.SetShowAvailablePackages(showOption);
	if (settings.FindBool(SETTING_SHOW_INSTALLED_PACKAGES, &showOption) == B_OK)
		fModel.SetShowInstalledPackages(showOption);
	if (settings.FindBool(SETTING_SHOW_DEVELOP_PACKAGES, &showOption) == B_OK)
		fModel.SetShowDevelopPackages(showOption);
	if (settings.FindBool(SETTING_SHOW_SOURCE_PACKAGES, &showOption) == B_OK)
		fModel.SetShowSourcePackages(showOption);
	if (settings.FindBool(SETTING_CAN_SHARE_ANONYMOUS_USER_DATA,
			&showOption) == B_OK) {
		fModel.SetCanShareAnonymousUsageData(showOption);
	}
}


void
MainWindow::_MaybePromptCanShareAnonymousUserData(const BMessage& settings)
{
	bool showOption;
	if (settings.FindBool(SETTING_CAN_SHARE_ANONYMOUS_USER_DATA,
			&showOption) == B_NAME_NOT_FOUND) {
		_PromptCanShareAnonymousUserData();
	}
}


void
MainWindow::_PromptCanShareAnonymousUserData()
{
	BAlert* alert = new(std::nothrow) BAlert(
		B_TRANSLATE("Sending anonymous usage data"),
		B_TRANSLATE("Would it be acceptable to send anonymous usage data to the"
			" HaikuDepotServer system from this computer? You can change your"
			" preference in the \"Settings\" window later."),
		B_TRANSLATE("No"),
		B_TRANSLATE("Yes"));

	int32 result = alert->Go();
	fModel.SetCanShareAnonymousUsageData(1 == result);
}


void
MainWindow::_InitWorkerThreads()
{
	fPackageToPopulateSem = create_sem(0, "PopulatePackage");
	if (fPackageToPopulateSem >= 0) {
		fPopulatePackageWorker = spawn_thread(&_PopulatePackageWorker,
			"Package Populator", B_NORMAL_PRIORITY, this);
		if (fPopulatePackageWorker >= 0)
			resume_thread(fPopulatePackageWorker);
	} else
		fPopulatePackageWorker = -1;
}


void
MainWindow::_AdoptModelControls()
{
	if (fSinglePackageMode)
		return;

	BAutolock locker(fModel.Lock());
	fShowAvailablePackagesItem->SetMarked(fModel.ShowAvailablePackages());
	fShowInstalledPackagesItem->SetMarked(fModel.ShowInstalledPackages());
	fShowSourcePackagesItem->SetMarked(fModel.ShowSourcePackages());
	fShowDevelopPackagesItem->SetMarked(fModel.ShowDevelopPackages());

	if (fModel.PackageListViewMode() == PROMINENT)
		fListTabs->Select(TAB_PROMINENT_PACKAGES);
	else
		fListTabs->Select(TAB_ALL_PACKAGES);

	fFilterView->AdoptModel(fModel);
}


void
MainWindow::_AdoptModel()
{
	HDTRACE("adopting model to main window ui");

	if (fSinglePackageMode)
		return;

	std::vector<DepotInfoRef> depots = _CreateSnapshotOfDepots();
	std::vector<DepotInfoRef>::iterator it;

	fFeaturedPackagesView->BeginAddRemove();

	for (it = depots.begin(); it != depots.end(); it++) {
		DepotInfoRef depotInfoRef = *it;
		for (int i = 0; i < depotInfoRef->CountPackages(); i++) {
			PackageInfoRef package = depotInfoRef->PackageAtIndex(i);
			_AddRemovePackageFromLists(package);
		}
	}

	fFeaturedPackagesView->EndAddRemove();

	_AdoptModelControls();
}


void
MainWindow::_AddRemovePackageFromLists(const PackageInfoRef& package)
{
	bool matches;

	{
		AutoLocker<BLocker> modelLocker(fModel.Lock());
		matches = fModel.MatchesFilter(package);
	}

	if (matches) {
		if (package->IsProminent())
			fFeaturedPackagesView->AddPackage(package);
		fPackageListView->AddPackage(package);
	} else {
		fFeaturedPackagesView->RemovePackage(package);
		fPackageListView->RemovePackage(package);
	}
}


void
MainWindow::_IncrementViewCounter(const PackageInfoRef& package)
{
	bool shouldIncrementViewCounter = false;

	{
		AutoLocker<BLocker> modelLocker(fModel.Lock());
		bool canShareAnonymousUsageData = fModel.CanShareAnonymousUsageData();
		if (canShareAnonymousUsageData && !package->Viewed()) {
			package->SetViewed();
			shouldIncrementViewCounter = true;
		}
	}

	if (shouldIncrementViewCounter) {
		ProcessCoordinator* bulkLoadCoordinator =
			ProcessCoordinatorFactory::CreateIncrementViewCounter(
				&fModel, package);
		_AddProcessCoordinator(bulkLoadCoordinator);
	}
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
MainWindow::_StartBulkLoad(bool force)
{
	if (fFeaturedPackagesView != NULL)
		fFeaturedPackagesView->Clear();
	if (fPackageListView != NULL)
		fPackageListView->Clear();
	fPackageInfoView->Clear();

	fRefreshRepositoriesItem->SetEnabled(false);
	ProcessCoordinator* bulkLoadCoordinator =
		ProcessCoordinatorFactory::CreateBulkLoadCoordinator(
			this,
				// PackageInfoListener
			&fModel, force);
	_AddProcessCoordinator(bulkLoadCoordinator);
}


void
MainWindow::_BulkLoadCompleteReceived(status_t errorStatus)
{
	if (errorStatus != B_OK) {
		AppUtils::NotifySimpleError(
			B_TRANSLATE("Package update error"),
			B_TRANSLATE("While updating package data, a problem has arisen "
				"that may cause data to be outdated or missing from the "
				"application's display. Additional details regarding this "
				"problem may be able to be obtained from the application "
				"logs."
				ALERT_MSG_LOGS_USER_GUIDE));
	}

	fRefreshRepositoriesItem->SetEnabled(true);
	_AdoptModel();
	_UpdateAvailableRepositories();

	// if after loading everything in, it transpires that there are no
	// featured packages then the featured packages should be disabled
	// and the user should be switched to the "all packages" view so
	// that they are not presented with a blank window!

	bool hasProminentPackages = fModel.HasAnyProminentPackages();
	fListTabs->TabAt(TAB_PROMINENT_PACKAGES)->SetEnabled(hasProminentPackages);
	if (!hasProminentPackages
			&& fListTabs->Selection() == TAB_PROMINENT_PACKAGES) {
		fModel.SetPackageListViewMode(ALL);
		fListTabs->Select(TAB_ALL_PACKAGES);
	}
}


void
MainWindow::_NotifyWorkStatusClear()
{
	BMessage message(MSG_WORK_STATUS_CLEAR);
	this->PostMessage(&message, this);
}


void
MainWindow::_HandleWorkStatusClear()
{
	fWorkStatusView->SetText("");
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
MainWindow::_HandleExternalPackageUpdateMessageReceived(const BMessage* message)
{
	BStringList addedPackageNames;
	BStringList removedPackageNames;

	if (message->FindStrings("added package names",
			&addedPackageNames) == B_OK) {
		addedPackageNames.Sort();
		AutoLocker<BLocker> locker(fModel.Lock());
		fModel.SetStateForPackagesByName(addedPackageNames, ACTIVATED);
	}
	else
		HDINFO("no [added package names] key in inbound message");

	if (message->FindStrings("removed package names",
			&removedPackageNames) == B_OK) {
		removedPackageNames.Sort();
		AutoLocker<BLocker> locker(fModel.Lock());
		fModel.SetStateForPackagesByName(addedPackageNames, UNINSTALLED);
	} else
		HDINFO("no [removed package names] key in inbound message");
}


void
MainWindow::_HandleWorkStatusChangeMessageReceived(const BMessage* message)
{
	if (fWorkStatusView == NULL)
		return;

	BString text;
	float progress;

	if (message->FindString(KEY_WORK_STATUS_TEXT, &text) == B_OK)
		fWorkStatusView->SetText(text);

	if (message->FindFloat(KEY_WORK_STATUS_PROGRESS, &progress) == B_OK) {
		if (progress < 0.0f)
			fWorkStatusView->SetBusy();
		else
			fWorkStatusView->SetProgress(progress);
	} else {
		HDERROR("work status change missing progress on update message");
		fWorkStatusView->SetProgress(0.0f);
	}
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

	HDDEBUG("pkg [%s] will be updated from the server.",
		fPackageToPopulate->Name().String());
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

		if (package.IsSet()) {
			uint32 populateFlags = Model::POPULATE_USER_RATINGS
				| Model::POPULATE_SCREEN_SHOTS
				| Model::POPULATE_CHANGELOG;

			if (force)
				populateFlags |= Model::POPULATE_FORCE;

			window->fModel.PopulatePackage(package, populateFlags);

			HDDEBUG("populating package [%s]", package->Name().String());
		}
	}

	return 0;
}


void
MainWindow::_OpenSettingsWindow()
{
	SettingsWindow* window = new SettingsWindow(this, &fModel);
	window->Show();
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
MainWindow::_StartUserVerify()
{
	if (!fModel.Nickname().IsEmpty()) {
		_AddProcessCoordinator(
			ProcessCoordinatorFactory::CreateUserDetailVerifierCoordinator(
				this,
					// UserDetailVerifierListener
				&fModel) );
	}
}


void
MainWindow::_UpdateAuthorization()
{
	BString nickname(fModel.Nickname());
	bool hasUser = !nickname.IsEmpty();

	if (fLogOutItem != NULL)
		fLogOutItem->SetEnabled(hasUser);
	if (fUsersUserUsageConditionsMenuItem != NULL)
		fUsersUserUsageConditionsMenuItem->SetEnabled(hasUser);
	if (fLogInItem != NULL) {
		if (hasUser)
			fLogInItem->SetLabel(B_TRANSLATE("Switch account" B_UTF8_ELLIPSIS));
		else
			fLogInItem->SetLabel(B_TRANSLATE("Log in" B_UTF8_ELLIPSIS));
	}

	if (fUserMenu != NULL) {
		BString label;
		if (hasUser) {
			label = B_TRANSLATE("Logged in as %User%");
			label.ReplaceAll("%User%", nickname);
		} else {
			label = B_TRANSLATE("Not logged in");
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
	std::vector<DepotInfoRef> depots = _CreateSnapshotOfDepots();
	std::vector<DepotInfoRef>::iterator it;

	for (it = depots.begin(); it != depots.end(); it++) {
		DepotInfoRef depot = *it;

		if (depot->Name().Length() != 0) {
			BMessage* message = new BMessage(MSG_DEPOT_SELECTED);
			message->AddString("name", depot->Name());
			BMenuItem* item = new(std::nothrow) BMenuItem(depot->Name(), message);

			if (item == NULL)
				HDFATAL("memory exhaustion");

			fRepositoryMenu->AddItem(item);

			if (depot->Name() == fModel.Depot()) {
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
		HDDEBUG("the package [%s] has no depot name", package->Name().String());
	} else {
		const DepotInfo* depot = fModel.DepotForName(depotName);

		if (depot == NULL) {
			HDINFO("the depot [%s] was not able to be found",
				depotName.String());
		} else {
			BString repositoryCode = depot->WebAppRepositoryCode();

			if (repositoryCode.IsEmpty()) {
				HDINFO("the depot [%s] has no web app repository code",
					depotName.String());
			} else
				return true;
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

	if (fModel.Nickname().IsEmpty()) {
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


void
MainWindow::_ViewUserUsageConditions(
	UserUsageConditionsSelectionMode mode)
{
	UserUsageConditionsWindow* window = new UserUsageConditionsWindow(
		fModel, mode);
	window->Show();
}


void
MainWindow::UserCredentialsFailed()
{
	BString message = B_TRANSLATE("The password previously "
		"supplied for the user [%Nickname%] is not currently "
		"valid. The user will be logged-out of this application "
		"and you should login again with your updated password.");
	message.ReplaceAll("%Nickname%", fModel.Nickname());

	AppUtils::NotifySimpleError(B_TRANSLATE("Login issue"),
		message);

	{
		AutoLocker<BLocker> locker(fModel.Lock());
		fModel.SetNickname("");
	}
}


/*! \brief This method is invoked from the UserDetailVerifierProcess on a
		   background thread.  For this reason it lodges a message into itself
		   which can then be handled on the main thread.
*/

void
MainWindow::UserUsageConditionsNotLatest(const UserDetail& userDetail)
{
	BMessage message(MSG_USER_USAGE_CONDITIONS_NOT_LATEST);
	BMessage detailsMessage;
	if (userDetail.Archive(&detailsMessage, true) != B_OK
			|| message.AddMessage("userDetail", &detailsMessage) != B_OK) {
		HDERROR("unable to archive the user detail into a message");
	}
	else
		BMessenger(this).SendMessage(&message);
}


void
MainWindow::_HandleUserUsageConditionsNotLatest(
	const UserDetail& userDetail)
{
	ToLatestUserUsageConditionsWindow* window =
		new ToLatestUserUsageConditionsWindow(this, fModel, userDetail);
	window->Show();
}


void
MainWindow::_AddProcessCoordinator(ProcessCoordinator* item)
{
	AutoLocker<BLocker> lock(&fCoordinatorLock);

	if (fShouldCloseWhenNoProcessesToCoordinate) {
		HDINFO("system shutting down --> new process coordinator [%s] rejected",
			item->Name().String());
		delete item;
		return;
	}

	item->SetListener(this);

	if (!fCoordinator.IsSet()) {
		if (acquire_sem(fCoordinatorRunningSem) != B_OK)
			debugger("unable to acquire the process coordinator sem");
		HDINFO("adding and starting a process coordinator [%s]",
			item->Name().String());
		fCoordinator = BReference<ProcessCoordinator>(item);
		fCoordinator->Start();
	}
	else {
		HDINFO("adding process coordinator [%s] to the queue",
			item->Name().String());
		fCoordinatorQueue.push(item);
	}
}


void
MainWindow::_SpinUntilProcessCoordinatorComplete()
{
	while (true) {
		if (acquire_sem(fCoordinatorRunningSem) != B_OK)
			debugger("unable to acquire the process coordinator sem");
		if (release_sem(fCoordinatorRunningSem) != B_OK)
			debugger("unable to release the process coordinator sem");
		{
			AutoLocker<BLocker> lock(&fCoordinatorLock);
			if (!fCoordinator.IsSet())
				return;
		}
	}
}


void
MainWindow::_StopProcessCoordinators()
{
	HDINFO("will stop all queued process coordinators");
	AutoLocker<BLocker> lock(&fCoordinatorLock);

	while (!fCoordinatorQueue.empty()) {
		BReference<ProcessCoordinator> processCoordinator
			= fCoordinatorQueue.front();
		HDINFO("will drop queued process coordinator [%s]",
			processCoordinator->Name().String());
		fCoordinatorQueue.pop();
	}

	if (fCoordinator.IsSet())
		fCoordinator->RequestStop();
}


/*! This method is called when there is some change in the bulk load process
	or other process coordinator.
	A change may mean that a new process has started / stopped etc... or it
	may mean that the entire coordinator has finished.
*/

void
MainWindow::CoordinatorChanged(ProcessCoordinatorState& coordinatorState)
{
	AutoLocker<BLocker> lock(&fCoordinatorLock);

	if (fCoordinator.Get() == coordinatorState.Coordinator()) {
		if (!coordinatorState.IsRunning()) {
			if (release_sem(fCoordinatorRunningSem) != B_OK)
				debugger("unable to release the process coordinator sem");
			HDINFO("process coordinator [%s] did complete",
				fCoordinator->Name().String());
			// complete the last one that just finished
			BMessage* message = fCoordinator->Message();

			if (message != NULL) {
				BMessenger messenger(this);
				message->AddInt64(KEY_ERROR_STATUS,
					(int64) fCoordinator->ErrorStatus());
				messenger.SendMessage(message);
			}

			fCoordinator = BReference<ProcessCoordinator>(NULL);
				// will delete the old process coordinator if it is not used
				// elsewhere.

			// now schedule the next one.
			if (!fCoordinatorQueue.empty()) {
				if (acquire_sem(fCoordinatorRunningSem) != B_OK)
					debugger("unable to acquire the process coordinator sem");
				fCoordinator = fCoordinatorQueue.front();
				HDINFO("starting next process coordinator [%s]",
					fCoordinator->Name().String());
				fCoordinatorQueue.pop();
				fCoordinator->Start();
			}
			else {
				_NotifyWorkStatusClear();
				if (fShouldCloseWhenNoProcessesToCoordinate) {
					HDINFO("no more processes to coord --> will quit");
					BMessage message(B_QUIT_REQUESTED);
					PostMessage(&message);
				}
			}
		}
		else {
			_NotifyWorkStatusChange(coordinatorState.Message(),
				coordinatorState.Progress());
				// show the progress to the user.
		}
	} else {
		_NotifyWorkStatusClear();
		HDINFO("! unknown process coordinator changed");
	}
}


static package_list_view_mode
main_window_tab_to_package_list_view_mode(int32 tab)
{
	if (tab == TAB_PROMINENT_PACKAGES)
		return PROMINENT;
	return ALL;
}


void
MainWindow::_HandleChangePackageListViewMode()
{
	package_list_view_mode tabMode = main_window_tab_to_package_list_view_mode(
		fListTabs->Selection());
	package_list_view_mode modelMode = fModel.PackageListViewMode();

	if (tabMode != modelMode) {
		BAutolock locker(fModel.Lock());
		fModel.SetPackageListViewMode(tabMode);
	}
}


std::vector<DepotInfoRef>
MainWindow::_CreateSnapshotOfDepots()
{
	std::vector<DepotInfoRef> result;
	BAutolock locker(fModel.Lock());
	int32 countDepots = fModel.CountDepots();
	for(int32 i = 0; i < countDepots; i++)
		result.push_back(fModel.DepotAtIndex(i));
	return result;
}
