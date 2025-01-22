/*
 * Copyright 2015, Axel Dörfler, <axeld@pinc-software.de>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "MainWindow.h"

#include <algorithm>
#include <map>
#include <vector>

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StopWatch.h>
#include <StringList.h>
#include <StringView.h>
#include <TabView.h>

#include "AppUtils.h"
#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "DecisionProvider.h"
#include "FeaturedPackagesView.h"
#include "FilterView.h"
#include "IdentityAndAccessUtils.h"
#include "LocaleUtils.h"
#include "Logger.h"
#include "PackageInfoView.h"
#include "PackageListView.h"
#include "PackageManager.h"
#include "PackageUtils.h"
#include "ProcessCoordinator.h"
#include "ProcessCoordinatorFactory.h"
#include "RatePackageWindow.h"
#include "RatingUtils.h"
#include "ScreenshotWindow.h"
#include "SettingsWindow.h"
#include "ShuttingDownWindow.h"
#include "ToLatestUserUsageConditionsWindow.h"
#include "UserLoginWindow.h"
#include "UserUsageConditionsWindow.h"
#include "WorkStatusView.h"
#include "support.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_REFRESH_REPOS						= 'mrrp',
	MSG_MANAGE_REPOS						= 'mmrp',
	MSG_SOFTWARE_UPDATER					= 'mswu',
	MSG_SETTINGS							= 'stgs',
	MSG_LOG_IN								= 'lgin',
	MSG_AUTHORIZATION_CHANGED				= 'athc',
	MSG_PACKAGE_FILTER_CHANGED				= 'fpch',
	MSG_ICONS_CHANGED						= 'icoc',
	MSG_CATEGORIES_LIST_CHANGED				= 'clic',
	MSG_PACKAGES_CHANGED					= 'pchd',
	MSG_PROCESS_COORDINATOR_CHANGED			= 'pccd',
	MSG_WORK_STATUS_CHANGE					= 'wsch',
	MSG_WORK_STATUS_CLEAR					= 'wscl',
	MSG_INCREMENT_VIEW_COUNTER				= 'icrv',
	MSG_SCREENSHOT_CACHED					= 'ssca',

	MSG_CHANGE_PACKAGE_LIST_VIEW_MODE		= 'cplm',
	MSG_SHOW_AVAILABLE_PACKAGES				= 'savl',
	MSG_SHOW_INSTALLED_PACKAGES				= 'sins',
	MSG_SHOW_SOURCE_PACKAGES				= 'ssrc',
	MSG_SHOW_DEVELOP_PACKAGES				= 'sdvl'
};

#define KEY_ERROR_STATUS				"errorStatus"

const bigtime_t kIncrementViewCounterDelayMicros = 3 * 1000 * 1000;

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

	virtual void PackageFilterChanged()
	{
		if (fMessenger.IsValid())
			fMessenger.SendMessage(MSG_PACKAGE_FILTER_CHANGED);
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

	virtual void ScreenshotCached(const ScreenshotCoordinate& coordinate)
	{
		if (fMessenger.IsValid()) {
			BMessage message(MSG_SCREENSHOT_CACHED);
			if (coordinate.Archive(&message) != B_OK)
				debugger("unable to serialize a screenshot coordinate");
			fMessenger.SendMessage(&message);
		}
	}

	virtual void IconsChanged()
	{
		if (fMessenger.IsValid())
			fMessenger.SendMessage(MSG_ICONS_CHANGED);
	}

private:
	BMessenger fMessenger;
};


class MainWindowPackageInfoListener : public PackageInfoListener {
public:
	MainWindowPackageInfoListener(MainWindow* mainWindow)
		:
		fMainWindow(mainWindow)
	{
	}

	~MainWindowPackageInfoListener() {}

private:
		// PackageInfoListener
	virtual void PackagesChanged(const PackageInfoEvents& events)
	{
		fMainWindow->PackagesChanged(events);
	}

private:
	MainWindow* fMainWindow;
};


MainWindow::MainWindow(const BMessage& settings)
	:
	BWindow(BRect(50, 50, 650, 550), B_TRANSLATE_SYSTEM_NAME("HaikuDepot"), B_DOCUMENT_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fScreenshotWindow(NULL),
	fShuttingDownWindow(NULL),
	fUserMenu(NULL),
	fLogInItem(NULL),
	fLogOutItem(NULL),
	fUsersUserUsageConditionsMenuItem(NULL),
	fModelListener(new MainWindowModelListener(BMessenger(this)), true),
	fCoordinator(NULL),
	fShouldCloseWhenNoProcessesToCoordinate(false),
	fSinglePackageMode(false),
	fIncrementViewCounterDelayedRunner(NULL)
{
	if ((fCoordinatorRunningSem = create_sem(1, "ProcessCoordinatorSem")) < B_OK)
		debugger("unable to create the process coordinator semaphore");

	_InitPreferredLanguage();

	fPackageInfoListener = PackageInfoListenerRef(new MainWindowPackageInfoListener(this), true);

	BMenuBar* menuBar = new BMenuBar("Main Menu");
	_BuildMenu(menuBar);

	BMenuBar* userMenuBar = new BMenuBar("User Menu");
	_BuildUserMenu(userMenuBar);
	set_small_font(userMenuBar);
	userMenuBar->SetExplicitMaxSize(BSize(B_SIZE_UNSET, menuBar->MaxSize().height));

	fFilterView = new FilterView();
	fFeaturedPackagesView = new FeaturedPackagesView(fModel);
	fPackageListView = new PackageListView(&fModel);
	fPackageInfoView = new PackageInfoView(&fModel, this);

	fSplitView = new BSplitView(B_VERTICAL, 5.0f);

	fWorkStatusView = new WorkStatusView("work status");
	fPackageListView->AttachWorkStatusView(fWorkStatusView);

	fListTabs
		= new TabView(BMessenger(this), BMessage(MSG_CHANGE_PACKAGE_LIST_VIEW_MODE), "list tabs");
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
	fModel.AddPackageListener(fPackageInfoListener);

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
	BPackageRoster().StartWatching(this, B_WATCH_PACKAGE_INSTALLATION_LOCATIONS);

	_AdoptModel();
	_StartBulkLoad();
}


/*!	This constructor is used when the application is loaded for the purpose of
	viewing an HPKG file.
*/

MainWindow::MainWindow(const BMessage& settings, const PackageInfoRef package)
	:
	BWindow(BRect(50, 50, 650, 350), B_TRANSLATE_SYSTEM_NAME("HaikuDepot"), B_DOCUMENT_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
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
	fSinglePackageMode(true),
	fIncrementViewCounterDelayedRunner(NULL)
{
	SetTitle(_WindowTitleForPackage(package));

	if ((fCoordinatorRunningSem = create_sem(1, "ProcessCoordinatorSem")) < B_OK)
		debugger("unable to create the process coordinator semaphore");

	_InitPreferredLanguage();

	fPackageInfoListener = PackageInfoListenerRef(new MainWindowPackageInfoListener(this), true);

	fFilterView = new FilterView();
	fPackageInfoView = new PackageInfoView(&fModel, this);
	fWorkStatusView = new WorkStatusView("work status");

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fPackageInfoView)
		.Add(fWorkStatusView)
		.SetInsets(0, B_USE_WINDOW_INSETS, 0, 0);

	fModel.AddListener(fModelListener);
	fModel.AddPackageListener(fPackageInfoListener);

	// add the single package into the model so that any internal
	// business logic is able to find the package.
	DepotInfoRef depot = DepotInfoBuilder()
							 .WithName(SINGLE_PACKAGE_DEPOT_NAME)
							 .WithIdentifier(SINGLE_PACKAGE_DEPOT_IDENTIFIER)
							 .BuildRef();

	PackageCoreInfoRef coreInfo = PackageCoreInfoBuilder(package->CoreInfo())
									  .WithDepotName(SINGLE_PACKAGE_DEPOT_IDENTIFIER)
									  .BuildRef();

	PackageInfoRef packageWithDepotIdentifier
		= PackageInfoBuilder(package).WithCoreInfo(coreInfo).BuildRef();

	fModel.SetDepots(depot);
	fModel.AddPackage(packageWithDepotIdentifier);

	// Restore settings
	_RestoreNickname(settings);
	_UpdateAuthorization();
	_RestoreWindowFrame(settings);

	fPackageInfoView->SetPackage(packageWithDepotIdentifier);

	// start worker threads
	BPackageRoster().StartWatching(this, B_WATCH_PACKAGE_INSTALLATION_LOCATIONS);
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

	// We must clear the model early to release references.
	fModel.Clear();
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

		if (fCoordinator != NULL) {
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
				_BulkLoadCompleteReceived(static_cast<status_t>(errorStatus64));
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
			if (IdentityAndAccessUtils::ClearCredentials() != B_OK)
				HDERROR("unable to remove stored credentials");
			fModel.SetCredentials(UserCredentials());
			break;

		case MSG_VIEW_LATEST_USER_USAGE_CONDITIONS:
			_ViewUserUsageConditions(LATEST);
			break;

		case MSG_VIEW_USERS_USER_USAGE_CONDITIONS:
			_ViewUserUsageConditions(USER);
			break;

		case MSG_AUTHORIZATION_CHANGED:
			if (!fSinglePackageMode)
				_StartUserVerify();
			_UpdateAuthorization();
			break;

		case MSG_ICONS_CHANGED:
			_HandleIconsChanged();
			break;

		case MSG_SCREENSHOT_CACHED:
			_HandleScreenshotCached(message);
			break;

		case MSG_CATEGORIES_LIST_CHANGED:
			fFilterView->AdoptModel(fModel);
			break;

		case MSG_PACKAGE_FILTER_CHANGED:
			// This message is originating from the model via a listener and
			// signals that specification of the package filter have changed
			// so that the application should re-filter the packages.
			_AdoptModel();
			break;

		case MSG_CHANGE_PACKAGE_LIST_VIEW_MODE:
			_HandleChangePackageListViewMode();
			break;

		case MSG_SHOW_AVAILABLE_PACKAGES:
		{
			fModel.SetFilterSpecification(
				PackageFilterSpecificationBuilder(fModel.FilterSpecification())
					.WithShowAvailablePackages(!fShowAvailablePackagesItem->IsMarked())
					// inverse because it should be toggled
					.BuildRef());
			break;
		}

		case MSG_SHOW_INSTALLED_PACKAGES:
		{
			fModel.SetFilterSpecification(
				PackageFilterSpecificationBuilder(fModel.FilterSpecification())
					.WithShowInstalledPackages(!fShowInstalledPackagesItem->IsMarked())
						// inverse because it should be toggled
					.BuildRef());
			break;
		}

		case MSG_SHOW_SOURCE_PACKAGES:
		{
			fModel.SetFilterSpecification(
				PackageFilterSpecificationBuilder(fModel.FilterSpecification())
					.WithShowSourcePackages(!fShowSourcePackagesItem->IsMarked())
					// inverse because it should be toggled
					.BuildRef());
			break;
		}

		case MSG_SHOW_DEVELOP_PACKAGES:
		{
			fModel.SetFilterSpecification(
				PackageFilterSpecificationBuilder(fModel.FilterSpecification())
					.WithShowDevelopPackages(!fShowDevelopPackagesItem->IsMarked())
					// inverse because it should be toggled
					.BuildRef());
			break;
		}

			// this may be triggered by, for example, a user rating being added
			// or having been altered.
		case MSG_SERVER_DATA_CHANGED:
		{
			BString name;
			if (message->FindString("name", &name) == B_OK) {
				if (fPackageInfoView->Package()->Name() == name) {
					_PopulatePackageAsync(true);
				} else {
					HDDEBUG("pkg [%s] is updated on the server, but is not selected so will not be "
							"updated.",
						name.String());
				}
			}
			break;
		}

		case MSG_INCREMENT_VIEW_COUNTER:
			_HandleIncrementViewCounter(message);
			break;

		case MSG_PACKAGE_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) == B_OK) {
				PackageInfoRef package;
				package = fModel.PackageForName(name);

				if (!package.IsSet() || name != package->Name()) {
					debugger("unable to find the named package");
				} else {
					_AdoptPackage(package);
					_SetupDelayedIncrementViewCounter(package);
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
			fModel.SetFilterSpecification(
				PackageFilterSpecificationBuilder(fModel.FilterSpecification())
					.WithCategory(code)
					.BuildRef());
			break;
		}

		case MSG_DEPOT_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				name = "";
			fModel.SetFilterSpecification(
				PackageFilterSpecificationBuilder(fModel.FilterSpecification())
					.WithDepotName(name)
					.BuildRef());
			_UpdateAvailableRepositories();
			break;
		}

		case MSG_SEARCH_TERMS_MODIFIED:
		{
			// TODO: Do this with a delay!
			BString searchTerms;
			if (message->FindString("search terms", &searchTerms) != B_OK)
				searchTerms = "";
			fModel.SetFilterSpecification(
				PackageFilterSpecificationBuilder(fModel.FilterSpecification())
					.WithSearchTerms(searchTerms)
					.BuildRef());
			break;
		}

		case MSG_PACKAGES_CHANGED:
			_HandlePackagesChanged(message);
			break;

		case MSG_PROCESS_COORDINATOR_CHANGED:
		{
			ProcessCoordinatorState state(message);
			_HandleProcessCoordinatorChanged(state);
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
			if (message->FindMessage("userDetail", &userDetailMsg) != B_OK)
				debugger("expected the [userDetail] data to be carried in the message.");
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
MainWindow::StoreSettings(BMessage& settings)
{
	settings.AddRect(_WindowFrameName(), Frame());
	if (!fSinglePackageMode) {
		settings.AddRect("window frame", Frame());

		BMessage columnSettings;
		if (fPackageListView != NULL)
			fPackageListView->SaveState(&columnSettings);

		settings.AddMessage("column settings", &columnSettings);

		settings.AddString(SETTING_PACKAGE_LIST_VIEW_MODE,
			main_window_package_list_view_mode_str(fModel.PackageListViewMode()));

		settings.AddBool(SETTING_SHOW_AVAILABLE_PACKAGES,
			fModel.FilterSpecification()->ShowAvailablePackages());
		settings.AddBool(SETTING_SHOW_INSTALLED_PACKAGES,
			fModel.FilterSpecification()->ShowInstalledPackages());
		settings.AddBool(SETTING_SHOW_DEVELOP_PACKAGES,
			fModel.FilterSpecification()->ShowDevelopPackages());
		settings.AddBool(SETTING_SHOW_SOURCE_PACKAGES,
			fModel.FilterSpecification()->ShowSourcePackages());

		settings.AddBool(SETTING_CAN_SHARE_ANONYMOUS_USER_DATA,
			fModel.CanShareAnonymousUsageData());
	}

	settings.AddString("username", fModel.Nickname());
}


void
MainWindow::Consume(ProcessCoordinator* item)
{
	_AddProcessCoordinator(item);
}


/*!	This method is invoked in the situation that a package changes in the model.

	The events are processed into a `BMessage` which is then posted to the
	window. The message is then trans-coded back into events for processing
	in the GUI.
*/
void
MainWindow::PackagesChanged(const PackageInfoEvents& events)
{
	BMessage message(MSG_PACKAGES_CHANGED);
	status_t result = events.Archive(&message);

	if (result == B_OK)
		PostMessage(&message);
	else
		HDERROR("unable to archive the package into events to a message");
}


/*!	This method is invoked as a result of the packages changing and the
	`MainWindow` finding out about this in the UI thread. From here it
	is able to inform other elements of the GUI about the change.
*/
void
MainWindow::_HandlePackagesChanged(const BMessage* message)
{
	PackageInfoEvents events;

	// Unpack the changes and at the same time marry them back up to the
	// package from the model.

	if (fModel.DearchiveInfoEvents(message, events) != B_OK) {
		HDERROR("unable to de-archive the package info events");
		return;
	}

	_HandlePackagesChanged(events);
}


void
MainWindow::_HandlePackagesChanged(const PackageInfoEvents& events)
{

	// if there are no events to process then drop.

	if (events.IsEmpty()) {
		HDINFO("window encountered an empty packages changed");
		return;
	}

	HDTRACE("window processing %" B_PRIi32 " package changes", events.CountEvents());

	// now process the messages by adding and removing them from lists.

	if (!fSinglePackageMode) {
		uint32 watchedChanges = PKG_CHANGED_LOCAL_INFO | PKG_CHANGED_CLASSIFICATION;
		PackageFilterRef filter = fModel.Filter();

		std::vector<PackageInfoRef> addedPackages;
		std::vector<PackageInfoRef> removedPackages;
		std::vector<PackageInfoRef> addedFeaturedPackages;
		std::vector<PackageInfoRef> removedFeaturedPackages;

		for (int32 i = events.CountEvents() - 1; i >= 0; i--) {
			const PackageInfoEvent event = events.EventAtIndex(i);

			if (event.Changes() & watchedChanges) {
				const PackageInfoRef package = event.Package();
				const bool isProminent = PackageUtils::IsProminent(package);

				if (package.IsSet()) {
					if (filter->AcceptsPackage(package)) {
						addedPackages.push_back(package);

						if (isProminent)
							addedFeaturedPackages.push_back(package);
					} else {
						removedPackages.push_back(package);

						if (isProminent)
							removedFeaturedPackages.push_back(package);
					}
				}
			}
		}

		fPackageListView->AddRemovePackages(addedPackages, removedPackages);
		fFeaturedPackagesView->AddRemovePackages(addedFeaturedPackages, removedFeaturedPackages);
	}

	// Now relay the changes to the other UI elements. This seems a bit strange
	// because we're controlling which packages are in the lists above, but then
	// updating anyway. This makes sense because the lists themselves should not
	// determine which packages are assigned.

	if (!fSinglePackageMode) {
		fFeaturedPackagesView->HandlePackagesChanged(events);
		fPackageListView->HandlePackagesChanged(events);
	}

	fPackageInfoView->HandlePackagesChanged(events);
}


void
MainWindow::_BuildMenu(BMenuBar* menuBar)
{
	BMenu* menu = new BMenu(B_TRANSLATE_SYSTEM_NAME("HaikuDepot"));
	fRefreshRepositoriesItem
		= new BMenuItem(B_TRANSLATE("Refresh repositories"), new BMessage(MSG_REFRESH_REPOS));
	menu->AddItem(fRefreshRepositoriesItem);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Manage repositories" B_UTF8_ELLIPSIS),
		new BMessage(MSG_MANAGE_REPOS)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Check for updates" B_UTF8_ELLIPSIS),
		new BMessage(MSG_SOFTWARE_UPDATER)));
	menu->AddSeparatorItem();
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS), new BMessage(MSG_SETTINGS), ','));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(menu);

	fRepositoryMenu = new BMenu(B_TRANSLATE("Repositories"));
	menuBar->AddItem(fRepositoryMenu);

	menu = new BMenu(B_TRANSLATE("Show"));

	fShowAvailablePackagesItem = new BMenuItem(B_TRANSLATE("Available packages"),
		new BMessage(MSG_SHOW_AVAILABLE_PACKAGES));
	menu->AddItem(fShowAvailablePackagesItem);

	fShowInstalledPackagesItem = new BMenuItem(B_TRANSLATE("Installed packages"),
		new BMessage(MSG_SHOW_INSTALLED_PACKAGES));
	menu->AddItem(fShowInstalledPackagesItem);

	menu->AddSeparatorItem();

	fShowDevelopPackagesItem
		= new BMenuItem(B_TRANSLATE("Develop packages"), new BMessage(MSG_SHOW_DEVELOP_PACKAGES));
	menu->AddItem(fShowDevelopPackagesItem);

	fShowSourcePackagesItem
		= new BMenuItem(B_TRANSLATE("Source packages"), new BMessage(MSG_SHOW_SOURCE_PACKAGES));
	menu->AddItem(fShowSourcePackagesItem);

	menuBar->AddItem(menu);
}


void
MainWindow::_BuildUserMenu(BMenuBar* menuBar)
{
	fUserMenu = new BMenu(B_TRANSLATE("Not logged in"));

	fLogInItem = new BMenuItem(B_TRANSLATE("Log in" B_UTF8_ELLIPSIS), new BMessage(MSG_LOG_IN));
	fUserMenu->AddItem(fLogInItem);

	fLogOutItem = new BMenuItem(B_TRANSLATE("Log out"), new BMessage(MSG_LOG_OUT));
	fUserMenu->AddItem(fLogOutItem);

	BMenuItem* latestUserUsageConditionsMenuItem
		= new BMenuItem(B_TRANSLATE("View latest usage conditions" B_UTF8_ELLIPSIS),
			new BMessage(MSG_VIEW_LATEST_USER_USAGE_CONDITIONS));
	fUserMenu->AddItem(latestUserUsageConditionsMenuItem);

	fUsersUserUsageConditionsMenuItem
		= new BMenuItem(B_TRANSLATE("View agreed usage conditions" B_UTF8_ELLIPSIS),
			new BMessage(MSG_VIEW_USERS_USER_USAGE_CONDITIONS));
	fUserMenu->AddItem(fUsersUserUsageConditionsMenuItem);

	menuBar->AddItem(fUserMenu);
}


void
MainWindow::_RestoreNickname(const BMessage& settings)
{
	BString nickname;
	if (settings.FindString("username", &nickname) == B_OK && nickname.Length() > 0) {
		UserCredentials credentials;
		if (IdentityAndAccessUtils::RetrieveCredentials(nickname, credentials) == B_OK) {
			fModel.SetCredentials(credentials);
			HDINFO("credentials restored");
		} else {
			HDERROR("the credentials are unable to be restored");
		}
	} else {
		HDINFO("no credentials to restore");
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
		if (width < screenFrame.Width() * .666f && height < screenFrame.Height() * .666f) {
			frame.bottom = frame.top + screenFrame.Height() * .666f;
			frame.right = frame.left + std::min(screenFrame.Width() * .666f, height * 7 / 5);
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
	if (settings.FindString(SETTING_PACKAGE_LIST_VIEW_MODE, &packageListViewMode) == B_OK) {
		fModel.SetPackageListViewMode(
			main_window_str_to_package_list_view_mode(packageListViewMode));
	}

	bool showOption;
	PackageFilterSpecificationBuilder filterSpecificationBuilder;

	if (settings.FindBool(SETTING_SHOW_AVAILABLE_PACKAGES, &showOption) == B_OK)
		filterSpecificationBuilder.WithShowAvailablePackages(showOption);
	if (settings.FindBool(SETTING_SHOW_INSTALLED_PACKAGES, &showOption) == B_OK)
		filterSpecificationBuilder.WithShowInstalledPackages(showOption);
	if (settings.FindBool(SETTING_SHOW_DEVELOP_PACKAGES, &showOption) == B_OK)
		filterSpecificationBuilder.WithShowDevelopPackages(showOption);
	if (settings.FindBool(SETTING_SHOW_SOURCE_PACKAGES, &showOption) == B_OK)
		filterSpecificationBuilder.WithShowSourcePackages(showOption);

	fModel.SetFilterSpecification(filterSpecificationBuilder.BuildRef());

	if (settings.FindBool(SETTING_CAN_SHARE_ANONYMOUS_USER_DATA, &showOption) == B_OK)
		fModel.SetCanShareAnonymousUsageData(showOption);
}


void
MainWindow::_MaybePromptCanShareAnonymousUserData(const BMessage& settings)
{
	bool showOption;
	if (settings.FindBool(SETTING_CAN_SHARE_ANONYMOUS_USER_DATA, &showOption) == B_NAME_NOT_FOUND)
		_PromptCanShareAnonymousUserData();
}


void
MainWindow::_PromptCanShareAnonymousUserData()
{
	BAlert* alert = new(std::nothrow) BAlert(B_TRANSLATE("Sending anonymous usage data"),
		B_TRANSLATE("Would it be acceptable to send anonymous usage data to the"
					" HaikuDepotServer system from this computer? You can change your"
					" preference in the \"Settings\" window later."),
		B_TRANSLATE("No"), B_TRANSLATE("Yes"));

	int32 result = alert->Go();
	fModel.SetCanShareAnonymousUsageData(1 == result);
}


/*!	This method will be invoked before reference data is loaded in; it is an
	initialization process.
*/
void
MainWindow::_InitPreferredLanguage()
{
	// TODO (apl) it would be nice to pre-detect the presence of a reference data
	//    file and load this early. See the logic in `ServerReferenceDataUpdateProcess`
	//    for details.
	const std::vector<LanguageRef> existingLanguages = fModel.Languages();
	LanguageRef defaultLanguage = LocaleUtils::DeriveDefaultLanguage(existingLanguages);
	std::vector<LanguageRef> languages(existingLanguages.begin(), existingLanguages.end());

	if (std::find(languages.begin(), languages.end(), defaultLanguage) == languages.end())
		languages.push_back(defaultLanguage);

	fModel.SetLanguagesAndPreferred(languages, defaultLanguage);
}


void
MainWindow::_AdoptModelControls()
{
	if (fSinglePackageMode)
		return;

	PackageFilterSpecificationRef packageFilterSpecification = fModel.FilterSpecification();

	fShowAvailablePackagesItem->SetMarked(packageFilterSpecification->ShowAvailablePackages());
	fShowInstalledPackagesItem->SetMarked(packageFilterSpecification->ShowInstalledPackages());
	fShowSourcePackagesItem->SetMarked(packageFilterSpecification->ShowSourcePackages());
	fShowDevelopPackagesItem->SetMarked(packageFilterSpecification->ShowDevelopPackages());

	if (fModel.PackageListViewMode() == PROMINENT)
		fListTabs->Select(TAB_PROMINENT_PACKAGES);
	else
		fListTabs->Select(TAB_ALL_PACKAGES);

	fFilterView->AdoptModel(fModel);
}


void
MainWindow::_AdoptModel()
{
	BStopWatch stopWatch("adopt_model", true);
	HDTRACE("adopting model to main window ui");

	if (fSinglePackageMode)
		return;

	// This list will contain all of the packages that are under consideration.
	// Create a new list which only contains the featured packages.

	const std::vector<PackageInfoRef> packages = _CreateSnapshotOfFilteredPackages();
	std::vector<PackageInfoRef> featuredPackages;
	std::vector<PackageInfoRef>::const_iterator it;

	for (it = packages.begin(); it != packages.end(); it++) {
		PackageInfoRef package = *it;
		if (PackageUtils::IsProminent(package))
			featuredPackages.push_back(package);
	}

	// Now get the UI elements which display those packages to only remain the
	// filtered list.

	fPackageListView->RetainPackages(packages);
	fFeaturedPackagesView->RetainPackages(featuredPackages);

	_AdoptModelControls();

	double durationSeconds = ((double)stopWatch.ElapsedTime() / 1000000.0);
	HDDEBUG("did adopt model in %6.3f seconds", durationSeconds);
}


void
MainWindow::_SetupDelayedIncrementViewCounter(const PackageInfoRef package)
{
	if (fIncrementViewCounterDelayedRunner != NULL) {
		fIncrementViewCounterDelayedRunner->SetCount(0);
		delete fIncrementViewCounterDelayedRunner;
	}
	BMessage message(MSG_INCREMENT_VIEW_COUNTER);
	message.SetString("name", package->Name());
	fIncrementViewCounterDelayedRunner
		= new BMessageRunner(BMessenger(this), &message, kIncrementViewCounterDelayMicros, 1);
	if (fIncrementViewCounterDelayedRunner->InitCheck() != B_OK)
		HDERROR("unable to init the increment view counter");
}


void
MainWindow::_HandleIncrementViewCounter(const BMessage* message)
{
	BString name;
	if (message->FindString("name", &name) == B_OK) {
		const PackageInfoRef& viewedPackage = fPackageInfoView->Package();
		if (viewedPackage.IsSet()) {
			if (viewedPackage->Name() == name)
				_IncrementViewCounter(viewedPackage);
			else
				HDINFO("incr. view counter; name mismatch");
		} else {
			HDINFO("incr. view counter; no viewed pkg");
		}
	} else {
		HDERROR("incr. view counter; no name");
	}
	fIncrementViewCounterDelayedRunner->SetCount(0);
	delete fIncrementViewCounterDelayedRunner;
	fIncrementViewCounterDelayedRunner = NULL;
}


void
MainWindow::_IncrementViewCounter(const PackageInfoRef package)
{
	bool shouldIncrementViewCounter = false;

	bool canShareAnonymousUsageData = fModel.CanShareAnonymousUsageData();
	if (canShareAnonymousUsageData && !PackageUtils::Viewed(package)) {
		fModel.AddPackage(PackageInfoBuilder(package)
				.WithLocalInfo(
					PackageLocalInfoBuilder(package->LocalInfo()).WithViewed().BuildRef())
				.BuildRef());
		shouldIncrementViewCounter = true;
	}

	if (shouldIncrementViewCounter) {
		ProcessCoordinator* incrementViewCoordinator
			= ProcessCoordinatorFactory::CreateIncrementViewCounter(&fModel, package);
		_AddProcessCoordinator(incrementViewCoordinator);
	}
}


void
MainWindow::_AdoptPackage(const PackageInfoRef& package)
{
	fPackageInfoView->SetPackage(package);

	if (!fSinglePackageMode) {
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
	if (!fSinglePackageMode) {
		if (fFeaturedPackagesView != NULL)
			fFeaturedPackagesView->Clear();
		if (fPackageListView != NULL)
			fPackageListView->Clear();
	}

	fPackageInfoView->Clear();

	fRefreshRepositoriesItem->SetEnabled(false);
	ProcessCoordinator* bulkLoadCoordinator
		= ProcessCoordinatorFactory::CreateBulkLoadCoordinator(&fModel, force);
	_AddProcessCoordinator(bulkLoadCoordinator);
}


void
MainWindow::_BulkLoadCompleteReceived(status_t errorStatus)
{
	if (errorStatus != B_OK) {
		AppUtils::NotifySimpleError(B_TRANSLATE("Package update error"),
			B_TRANSLATE("While updating package data, a problem has arisen "
						"that may cause data to be outdated or missing from the "
						"application's display. Additional details regarding this "
						"problem may be able to be obtained from the application "
						"logs." ALERT_MSG_LOGS_USER_GUIDE));
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
	if (!hasProminentPackages && fListTabs->Selection() == TAB_PROMINENT_PACKAGES) {
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


/*!	Sends off a message to the Window so that it can change the status view
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

	if (message->FindStrings("added package names", &addedPackageNames) == B_OK) {
		addedPackageNames.Sort();
		_SetStateForPackagesByName(addedPackageNames, ACTIVATED);
	} else {
		HDINFO("no [added package names] key in inbound message");
	}

	if (message->FindStrings("removed package names", &removedPackageNames) == B_OK) {
		removedPackageNames.Sort();
		_SetStateForPackagesByName(removedPackageNames, UNINSTALLED);
	} else {
		HDINFO("no [removed package names] key in inbound message");
	}
}


void
MainWindow::_SetStateForPackagesByName(BStringList& packageNames, PackageState state)
{
	std::vector<PackageInfoRef> modifiedPackages;

	for (int32 i = 0; i < packageNames.CountStrings(); i++) {
		BString packageName = packageNames.StringAt(i);
		const PackageInfoRef package = fModel.PackageForName(packageName);

		if (package.IsSet()) {
			PackageLocalInfoRef localInfo
				= PackageLocalInfoBuilder(package->LocalInfo()).WithState(state).BuildRef();

			modifiedPackages.push_back(
				PackageInfoBuilder(package).WithLocalInfo(localInfo).BuildRef());

			HDINFO("will update package [%s] with state [%s]", packageName.String(),
				PackageUtils::StateToString(state));
		} else {
			HDINFO("was unable to find package [%s] so was not possible to set the state to [%s]",
				packageName.String(), PackageUtils::StateToString(state));
		}
	}

	fModel.AddPackagesWithChange(modifiedPackages, PKG_CHANGED_LOCAL_INFO);
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


/*! Initially only superficial data is loaded from the server into the data
	model of the packages.  When the package is viewed, additional data needs
	to be populated including ratings.

	This method will cause the package to have its data refreshed from
	the server application.  The refresh happens in the background; this method
	is asynchronous.
*/

void
MainWindow::_PopulatePackageAsync(bool forcePopulate)
{
	const PackageInfoRef package = fPackageInfoView->Package();

	if (!fModel.CanPopulatePackage(package))
		return;

	const char* packageNameStr = package->Name().String();

	PackageLocalizedTextRef localized = package->LocalizedText();

	if (localized.IsSet()) {
		if (localized->HasChangelog() && (forcePopulate || localized->Changelog().IsEmpty())) {
			_AddProcessCoordinator(ProcessCoordinatorFactory::PopulatePkgChangelogCoordinator(
				&fModel, package->Name()));
			HDINFO("pkg [%s] will have changelog updated from server.", packageNameStr);
		} else {
			HDDEBUG("pkg [%s] not have changelog updated from server.", packageNameStr);
		}
	}

	if (forcePopulate || RatingUtils::ShouldTryPopulateUserRatings(package->UserRatingInfo())) {
		_AddProcessCoordinator(
			ProcessCoordinatorFactory::PopulatePkgUserRatingsCoordinator(&fModel, package->Name()));
		HDINFO("pkg [%s] will have user ratings updated from server.", packageNameStr);
	} else {
		HDDEBUG("pkg [%s] not have user ratings updated from server.", packageNameStr);
	}
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
	UserLoginWindow* window = new UserLoginWindow(this, BRect(0, 0, 500, 400), fModel);

	if (onSuccessMessage.what != 0)
		window->SetOnSuccessMessage(BMessenger(this), onSuccessMessage);

	window->Show();
}


void
MainWindow::_StartUserVerify()
{
	if (!fModel.Nickname().IsEmpty()) {
		_AddProcessCoordinator(ProcessCoordinatorFactory::CreateUserDetailVerifierCoordinator(this,
			// UserDetailVerifierListener
			&fModel));
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

	fRepositoryMenu->AddItem(
		new BMenuItem(B_TRANSLATE("All repositories"), new BMessage(MSG_DEPOT_SELECTED)));

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

			if (depot->Name() == fModel.FilterSpecification()->DepotName()) {
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
	const BString depotName = PackageUtils::DepotName(package);

	if (depotName.IsEmpty()) {
		HDDEBUG("the package [%s] has no depot name", package->Name().String());
	} else {
		const DepotInfo* depot = fModel.DepotForName(depotName);

		if (depot == NULL) {
			HDINFO("the depot [%s] was not able to be found", depotName.String());
		} else {
			BString repositoryCode = depot->WebAppRepositoryCode();

			if (repositoryCode.IsEmpty())
				HDINFO("the depot [%s] has no web app repository code", depotName.String());
			else
				return true;
		}
	}

	return false;
}


void
MainWindow::_RatePackage()
{
	if (!_SelectedPackageHasWebAppRepositoryCode()) {
		BAlert* alert = new(std::nothrow) BAlert(B_TRANSLATE("Rating not possible"),
			B_TRANSLATE("This package doesn't seem to be on the HaikuDepot Server, so it's not "
						"possible to create a new rating or edit an existing rating."),
			B_TRANSLATE("OK"));
		alert->Go();
		return;
	}

	if (fModel.Nickname().IsEmpty()) {
		BAlert* alert = new(std::nothrow) BAlert(B_TRANSLATE("Not logged in"),
			B_TRANSLATE("You need to be logged into an account before you can rate packages."),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Login or Create account"));

		if (alert == NULL)
			return;

		int32 choice = alert->Go();
		if (choice == 1)
			_OpenLoginWindow(BMessage(MSG_RATE_PACKAGE));
		return;
	}

	// TODO: Allow only one RatePackageWindow
	// TODO: Mechanism for remembering the window frame
	RatePackageWindow* window = new RatePackageWindow(this, BRect(0, 0, 500, 400), fModel);
	window->SetPackage(fPackageInfoView->Package());
	window->Show();
}


void
MainWindow::_ShowScreenshot()
{
	// TODO: Mechanism for remembering the window frame
	if (fScreenshotWindow == NULL)
		fScreenshotWindow = new ScreenshotWindow(this, BRect(0, 0, 500, 400), &fModel);

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
MainWindow::_ViewUserUsageConditions(UserUsageConditionsSelectionMode mode)
{
	UserUsageConditionsWindow* window = new UserUsageConditionsWindow(fModel, mode);
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

	AppUtils::NotifySimpleError(B_TRANSLATE("Login issue"), message);

	if (IdentityAndAccessUtils::ClearCredentials() != B_OK)
		HDERROR("unable to remove stored credentials");
	fModel.SetCredentials(UserCredentials());
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
	} else {
		BMessenger(this).SendMessage(&message);
	}
}


void
MainWindow::_HandleUserUsageConditionsNotLatest(const UserDetail& userDetail)
{
	ToLatestUserUsageConditionsWindow* window
		= new ToLatestUserUsageConditionsWindow(this, fModel, userDetail);
	window->Show();
}


void
MainWindow::_AddProcessCoordinator(ProcessCoordinator* item)
{
	AutoLocker<BLocker> lock(&fCoordinatorLock);

	if (fShouldCloseWhenNoProcessesToCoordinate) {
		HDINFO("system shutting down --> new process coordinator [%s] rejected",
			item->Name().String());
		return;
	}

	item->SetListener(this);

	if (fCoordinator == NULL) {
		if (acquire_sem(fCoordinatorRunningSem) != B_OK)
			debugger("unable to acquire the process coordinator sem");
		HDINFO("adding and starting a process coordinator [%s]", item->Name().String());
		delete fCoordinator;
		fCoordinator = item;
		fCoordinator->Start();
	} else {
		HDINFO("adding process coordinator [%s] to the queue", item->Name().String());
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
			if (fCoordinator == NULL)
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
		ProcessCoordinator* processCoordinator = fCoordinatorQueue.front();
		HDINFO("will drop queued process coordinator [%s]", processCoordinator->Name().String());
		fCoordinatorQueue.pop();
		delete processCoordinator;
	}

	if (fCoordinator != NULL)
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
	BMessage message(MSG_PROCESS_COORDINATOR_CHANGED);
	if (coordinatorState.Archive(&message, true) != B_OK)
		HDFATAL("unable to archive message when the process coordinator has changed");
	BMessenger(this).SendMessage(&message);
}


void
MainWindow::_HandleProcessCoordinatorChanged(ProcessCoordinatorState& coordinatorState)
{
	AutoLocker<BLocker> lock(&fCoordinatorLock);

	if (fCoordinator->Identifier() == coordinatorState.ProcessCoordinatorIdentifier()) {
		if (!coordinatorState.IsRunning()) {
			if (release_sem(fCoordinatorRunningSem) != B_OK)
				debugger("unable to release the process coordinator sem");
			HDINFO("process coordinator [%s] did complete", fCoordinator->Name().String());
			// complete the last one that just finished
			BMessage* message = fCoordinator->Message();

			if (message != NULL) {
				BMessenger messenger(this);
				message->AddInt64(KEY_ERROR_STATUS,
					static_cast<int64>(fCoordinator->ErrorStatus()));
				messenger.SendMessage(message);
			}

			HDDEBUG("process coordinator report;\n---\n%s\n----",
				fCoordinator->LogReport().String());

			delete fCoordinator;
			fCoordinator = NULL;

			// now schedule the next one.
			if (!fCoordinatorQueue.empty()) {
				if (acquire_sem(fCoordinatorRunningSem) != B_OK)
					debugger("unable to acquire the process coordinator sem");
				fCoordinator = fCoordinatorQueue.front();
				HDINFO("starting next process coordinator [%s]", fCoordinator->Name().String());
				fCoordinatorQueue.pop();
				fCoordinator->Start();
			} else {
				_NotifyWorkStatusClear();
				if (fShouldCloseWhenNoProcessesToCoordinate) {
					HDINFO("no more processes to coord --> will quit");
					BMessage message(B_QUIT_REQUESTED);
					PostMessage(&message);
				}
			}
		} else {
			_NotifyWorkStatusChange(coordinatorState.Message(), coordinatorState.Progress());
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
	package_list_view_mode tabMode
		= main_window_tab_to_package_list_view_mode(fListTabs->Selection());
	package_list_view_mode modelMode = fModel.PackageListViewMode();

	if (tabMode != modelMode)
		fModel.SetPackageListViewMode(tabMode);
}


/*!	This method will create a vector of packages that are filtered by the filter
	defined in the model.
*/
const std::vector<PackageInfoRef>
MainWindow::_CreateSnapshotOfFilteredPackages()
{
	return fModel.FilteredPackages();
}


std::vector<DepotInfoRef>
MainWindow::_CreateSnapshotOfDepots()
{
	return fModel.Depots();
}


/*!	This method will get invoked in the case that the icons have been
	reloaded. It applies to all packages because the icons are loaded
	all in one go.
*/

void
MainWindow::_HandleIconsChanged()
{
	if (!fSinglePackageMode) {
		fFeaturedPackagesView->HandleIconsChanged();
		fPackageListView->HandleIconsChanged();
	}

	fPackageInfoView->HandleIconsChanged();
}


/*!	This will get invoked in the case that a screenshot has been cached
	and so could now be loaded by some UI element. This method will then
	signal to other UI elements that they could load a screenshot should
	they have been waiting for it.
*/

void
MainWindow::_HandleScreenshotCached(const BMessage* message)
{
	ScreenshotCoordinate coordinate(message);
	fPackageInfoView->HandleScreenshotCached(coordinate);
}


/*static*/ const BString
MainWindow::_WindowTitleForPackage(const PackageInfoRef& pkg)
{
	PackageVersionRef version = PackageUtils::Version(pkg);
	BString versionString = "???";

	if (version.IsSet())
		versionString = version->ToString();

	BString title = B_TRANSLATE("HaikuDepot - %PackageName% %PackageVersion%");
	title.ReplaceAll("%PackageName%", pkg->Name());
	title.ReplaceAll("%PackageVersion%", versionString);

	return title;
}
