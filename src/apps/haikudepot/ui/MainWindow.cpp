/*
 * Copyright 2015, Axel Dörfler, <axeld@pinc-software.de>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2016, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "MainWindow.h"

#include <map>

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

#include <package/Context.h>
#include <package/manager/Exceptions.h>
#include <package/manager/RepositoryBuilder.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/PackageRoster.h>
#include "package/RepositoryCache.h"
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>
#include <package/solver/SolverRepository.h>
#include <package/solver/SolverResult.h>

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "DecisionProvider.h"
#include "FeaturedPackagesView.h"
#include "FilterView.h"
#include "JobStateListener.h"
#include "PackageInfoView.h"
#include "PackageListView.h"
#include "PackageManager.h"
#include "RatePackageWindow.h"
#include "support.h"
#include "ScreenshotWindow.h"
#include "UserLoginWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_MODEL_WORKER_DONE		= 'mmwd',
	MSG_REFRESH_REPOS			= 'mrrp',
	MSG_MANAGE_REPOS			= 'mmrp',
	MSG_LOG_IN					= 'lgin',
	MSG_LOG_OUT					= 'lgot',
	MSG_AUTHORIZATION_CHANGED	= 'athc',
	MSG_PACKAGE_CHANGED			= 'pchd',

	MSG_SHOW_FEATURED_PACKAGES	= 'sofp',
	MSG_SHOW_AVAILABLE_PACKAGES	= 'savl',
	MSG_SHOW_INSTALLED_PACKAGES	= 'sins',
	MSG_SHOW_SOURCE_PACKAGES	= 'ssrc',
	MSG_SHOW_DEVELOP_PACKAGES	= 'sdvl'
};


using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;


typedef std::map<BString, PackageInfoRef> PackageInfoMap;
typedef std::map<BString, DepotInfo> DepotInfoMap;


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
	fTerminating(false),
	fSinglePackageMode(false),
	fModelWorker(B_BAD_THREAD_ID)
{
	BMenuBar* menuBar = new BMenuBar(B_TRANSLATE("Main Menu"));
	_BuildMenu(menuBar);

	BMenuBar* userMenuBar = new BMenuBar(B_TRANSLATE("User Menu"));
	_BuildUserMenu(userMenuBar);
	set_small_font(userMenuBar);
	userMenuBar->SetExplicitMaxSize(BSize(B_SIZE_UNSET,
		menuBar->MaxSize().height));

	fFilterView = new FilterView();
	fFeaturedPackagesView = new FeaturedPackagesView();
	fPackageListView = new PackageListView(fModel.Lock());
	fPackageInfoView = new PackageInfoView(fModel.Lock(), this);

	fSplitView = new BSplitView(B_VERTICAL, 5.0f);

	BGroupView* featuredPackagesGroup = new BGroupView(B_VERTICAL);
	BStringView* featuredPackagesTitle = new BStringView(
		"featured packages title", B_TRANSLATE("Featured packages"));
	BFont font(be_bold_font);
	font.SetSize(font.Size() * 1.3f);
	featuredPackagesTitle->SetFont(&font);
	featuredPackagesGroup->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BLayoutBuilder::Group<>(featuredPackagesGroup)
		.Add(featuredPackagesTitle)
		.Add(fFeaturedPackagesView)
	;

	BView* listArea = new BView("list area", 0);
	fListLayout = new BCardLayout();
	listArea->SetLayout(fListLayout);
	listArea->AddChild(featuredPackagesGroup);
	listArea->AddChild(fPackageListView);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(menuBar, 1.0f)
			.Add(userMenuBar, 0.0f)
		.End()
		.Add(fFilterView)
		.AddSplit(fSplitView)
			.AddGroup(B_VERTICAL)
				.Add(listArea)
				.SetInsets(
					B_USE_DEFAULT_SPACING, 0.0f,
					B_USE_DEFAULT_SPACING, 0.0f)
			.End()
			.Add(fPackageInfoView)
		.End()
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
		fListLayout->SetVisibleItem((int32)0);
	else
		fListLayout->SetVisibleItem(1);

	_RestoreUserName(settings);
	_RestoreWindowFrame(settings);

	// start worker threads
	BPackageRoster().StartWatching(this,
		B_WATCH_PACKAGE_INSTALLATION_LOCATIONS);

	_StartRefreshWorker();

	_InitWorkerThreads();
}


MainWindow::MainWindow(const BMessage& settings, const PackageInfoRef& package)
	:
	BWindow(BRect(50, 50, 650, 350), B_TRANSLATE_SYSTEM_NAME("HaikuDepot"),
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fScreenshotWindow(NULL),
	fUserMenu(NULL),
	fLogInItem(NULL),
	fLogOutItem(NULL),
	fModelListener(new MessageModelListener(BMessenger(this)), true),
	fTerminating(false),
	fSinglePackageMode(true),
	fModelWorker(B_BAD_THREAD_ID)
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

	fTerminating = true;
	if (fModelWorker >= 0)
		wait_for_thread(fModelWorker, NULL);

	delete_sem(fPendingActionsSem);
	if (fPendingActionsWorker >= 0)
		wait_for_thread(fPendingActionsWorker, NULL);

	delete_sem(fPackageToPopulateSem);
	if (fPopulatePackageWorker >= 0)
		wait_for_thread(fPopulatePackageWorker, NULL);

	if (fScreenshotWindow != NULL && fScreenshotWindow->Lock())
		fScreenshotWindow->Quit();
}


bool
MainWindow::QuitRequested()
{
	BMessage settings;
	StoreSettings(settings);

	BMessage message(MSG_MAIN_WINDOW_CLOSED);
	message.AddMessage("window settings", &settings);

	be_app->PostMessage(&message);

	return true;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MODEL_WORKER_DONE:
		{
			fModelWorker = B_BAD_THREAD_ID;
			_AdoptModel();
			fFilterView->AdoptModel(fModel);
			break;
		}
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			// TODO: ?
			break;

		case B_PACKAGE_UPDATE:
			// TODO: We should do a more selective update depending on the
			// "event", "location", and "change count" fields!
			_StartRefreshWorker(false);
			break;

		case MSG_REFRESH_REPOS:
			_StartRefreshWorker(true);
			break;

		case MSG_MANAGE_REPOS:
			be_roster->Launch("application/x-vnd.Haiku-Repositories");
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
			{
				BAutolock locker(fModel.Lock());
				fModel.SetShowFeaturedPackages(
					!fModel.ShowFeaturedPackages());
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
					PackageList visiblePackages = fModel.CreatePackageList();
					isVisible = visiblePackages.Contains(ref);

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
			}
			break;
		}

		case MSG_RATE_PACKAGE:
			_RatePackage();
			break;

		case MSG_SHOW_SCREENSHOT:
			_ShowScreenshot();
			break;

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
	uint32 whatchedChanges = PKG_CHANGED_STATE | PKG_CHANGED_PROMINENCE;
	if ((event.Changes() & whatchedChanges) != 0) {
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
	menu->AddItem(new BMenuItem(B_TRANSLATE("Refresh repositories"),
			new BMessage(MSG_REFRESH_REPOS)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Manage repositories"),
			new BMessage(MSG_MANAGE_REPOS)));

	menuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Show"));

	fShowFeaturedPackagesItem = new BMenuItem(
		B_TRANSLATE("Only featured packages"),
		new BMessage(MSG_SHOW_FEATURED_PACKAGES));
	menu->AddItem(fShowFeaturedPackagesItem);

	menu->AddSeparatorItem();

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
}


void
MainWindow::_AdoptModel()
{
	fVisiblePackages = fModel.CreatePackageList();

	fFeaturedPackagesView->Clear();
	fPackageListView->Clear();
	for (int32 i = 0; i < fVisiblePackages.CountItems(); i++) {
		BAutolock locker(fModel.Lock());

		const PackageInfoRef& package = fVisiblePackages.ItemAtFast(i);
		fPackageListView->AddPackage(package);

		if (package->IsProminent())
			fFeaturedPackagesView->AddPackage(package);
	}

	BAutolock locker(fModel.Lock());
	fShowFeaturedPackagesItem->SetMarked(fModel.ShowFeaturedPackages());
	fShowFeaturedPackagesItem->SetEnabled(fModel.SearchTerms() == "");
	fShowAvailablePackagesItem->SetMarked(fModel.ShowAvailablePackages());
	fShowInstalledPackagesItem->SetMarked(fModel.ShowInstalledPackages());
	fShowSourcePackagesItem->SetMarked(fModel.ShowSourcePackages());
	fShowDevelopPackagesItem->SetMarked(fModel.ShowDevelopPackages());

	if (fModel.ShowFeaturedPackages() && fModel.SearchTerms() == "")
		fListLayout->SetVisibleItem((int32)0);
	else
		fListLayout->SetVisibleItem((int32)1);

	// Maintain selection
	const PackageInfoRef& selectedPackage = fPackageInfoView->Package();
	fFeaturedPackagesView->SelectPackage(selectedPackage);
	fPackageListView->SelectPackage(selectedPackage);

	if (!fVisiblePackages.Contains(fPackageInfoView->Package()))
		fPackageInfoView->Clear();
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

	// Trigger asynchronous package population from the web-app
	{
		AutoLocker<BLocker> lock(&fPackageToPopulateLock);
		fPackageToPopulate = package;
	}
	release_sem_etc(fPackageToPopulateSem, 1, 0);
}


void
MainWindow::_ClearPackage()
{
	fPackageInfoView->Clear();
}


void
MainWindow::_RefreshRepositories(bool force)
{
	if (fSinglePackageMode)
		return;

	BPackageRoster roster;
	BStringList repositoryNames;

	status_t result = roster.GetRepositoryNames(repositoryNames);
	if (result != B_OK)
		return;

	DecisionProvider decisionProvider;
	JobStateListener listener;
	BContext context(decisionProvider, listener);

	BRepositoryCache cache;
	for (int32 i = 0; i < repositoryNames.CountStrings(); ++i) {
		const BString& repoName = repositoryNames.StringAt(i);
		BRepositoryConfig repoConfig;
		result = roster.GetRepositoryConfig(repoName, &repoConfig);
		if (result != B_OK) {
			// TODO: notify user
			continue;
		}

		if (roster.GetRepositoryCache(repoName, &cache) != B_OK || force) {
			try {
				BRefreshRepositoryRequest refreshRequest(context, repoConfig);

				result = refreshRequest.Process();
			} catch (BFatalErrorException ex) {
				BString message(B_TRANSLATE("An error occurred while "
					"refreshing the repository: %error% (%details%)"));
 				message.ReplaceFirst("%error%", ex.Message());
				message.ReplaceFirst("%details%", ex.Details());
				_NotifyUser("Error", message.String());
			} catch (BException ex) {
				BString message(B_TRANSLATE("An error occurred while "
					"refreshing the repository: %error%"));
				message.ReplaceFirst("%error%", ex.Message());
				_NotifyUser("Error", message.String());
			}
		}
	}
}


void
MainWindow::_RefreshPackageList(bool force)
{
	if (fSinglePackageMode)
		return;

	BPackageRoster roster;
	BStringList repositoryNames;

	status_t result = roster.GetRepositoryNames(repositoryNames);
	if (result != B_OK)
		return;

	DepotInfoMap depots;
	for (int32 i = 0; i < repositoryNames.CountStrings(); i++) {
		const BString& repoName = repositoryNames.StringAt(i);
		DepotInfo depotInfo = DepotInfo(repoName);

		BRepositoryConfig repoConfig;
		status_t getRepositoryConfigStatus = roster.GetRepositoryConfig(
			repoName, &repoConfig);

		if (getRepositoryConfigStatus == B_OK) {
			depotInfo.SetBaseURL(repoConfig.BaseURL());

			// it would be nice if this could be more logically located such as
			// when the repository is added to the model, but that is probably
			// a bigger change.

			fModel.PopulateWebAppRepositoryCode(depotInfo);
		} else {
			printf("unable to obtain the repository config for local "
				"repository '%s'; %s\n",
				repoName.String(), strerror(getRepositoryConfigStatus));
		}

		depots[repoName] = depotInfo;
	}

	PackageManager manager(B_PACKAGE_INSTALLATION_LOCATION_HOME);
	try {
		manager.Init(PackageManager::B_ADD_INSTALLED_REPOSITORIES
			| PackageManager::B_ADD_REMOTE_REPOSITORIES);
	} catch (BException ex) {
		BString message(B_TRANSLATE("An error occurred while "
			"initializing the package manager: %message%"));
		message.ReplaceFirst("%message%", ex.Message());
		_NotifyUser("Error", message.String());
		return;
	}

	BObjectList<BSolverPackage> packages;
	result = manager.Solver()->FindPackages("",
		BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME
			| BSolver::B_FIND_IN_SUMMARY | BSolver::B_FIND_IN_DESCRIPTION
			| BSolver::B_FIND_IN_PROVIDES,
		packages);
	if (result != B_OK) {
		BString message(B_TRANSLATE("An error occurred while "
			"obtaining the package list: %message%"));
		message.ReplaceFirst("%message%", strerror(result));
		_NotifyUser("Error", message.String());
		return;
	}

	if (packages.IsEmpty())
		return;

	PackageInfoMap foundPackages;
		// if a given package is installed locally, we will potentially
		// get back multiple entries, one for each local installation
		// location, and one for each remote repository the package
		// is available in. The above map is used to ensure that in such
		// cases we consolidate the information, rather than displaying
		// duplicates
	PackageInfoMap remotePackages;
		// any package that we find in a remote repository goes in this map.
		// this is later used to discern which packages came from a local
		// installation only, as those must be handled a bit differently
		// upon uninstallation, since we'd no longer be able to pull them
		// down remotely.
	BStringList systemFlaggedPackages;
		// any packages flagged as a system package are added to this list.
		// such packages cannot be uninstalled, nor can any of their deps.
	PackageInfoMap systemInstalledPackages;
		// any packages installed in system are added to this list.
		// This is later used for dependency resolution of the actual
		// system packages in order to compute the list of protected
		// dependencies indicated above.

	for (int32 i = 0; i < packages.CountItems(); i++) {
		BSolverPackage* package = packages.ItemAt(i);
		const BPackageInfo& repoPackageInfo = package->Info();
		const BString repositoryName = package->Repository()->Name();
		PackageInfoRef modelInfo;
		PackageInfoMap::iterator it = foundPackages.find(
			repoPackageInfo.Name());
		if (it != foundPackages.end())
			modelInfo.SetTo(it->second);
		else {
			// Add new package info
			modelInfo.SetTo(new(std::nothrow) PackageInfo(repoPackageInfo),
				true);

			if (modelInfo.Get() == NULL)
				return;

			modelInfo->SetDepotName(repositoryName);

			foundPackages[repoPackageInfo.Name()] = modelInfo;
		}

		modelInfo->AddListener(this);

		BSolverRepository* repository = package->Repository();
		if (dynamic_cast<BPackageManager::RemoteRepository*>(repository)
				!= NULL) {
			depots[repository->Name()].AddPackage(modelInfo);
			remotePackages[modelInfo->Name()] = modelInfo;
		} else {
			if (repository == static_cast<const BSolverRepository*>(
					manager.SystemRepository())) {
				modelInfo->AddInstallationLocation(
					B_PACKAGE_INSTALLATION_LOCATION_SYSTEM);
				if (!modelInfo->IsSystemPackage()) {
					systemInstalledPackages[repoPackageInfo.FileName()]
						= modelInfo;
				}
			} else if (repository == static_cast<const BSolverRepository*>(
					manager.HomeRepository())) {
				modelInfo->AddInstallationLocation(
					B_PACKAGE_INSTALLATION_LOCATION_HOME);
			}
		}

		if (modelInfo->IsSystemPackage())
			systemFlaggedPackages.Add(repoPackageInfo.FileName());
	}

	bool wasEmpty = fModel.Depots().IsEmpty();
	if (force || wasEmpty)
		fModel.StopPopulatingAllPackages();

	BAutolock lock(fModel.Lock());

	if (force)
		fModel.Clear();

	// filter remote packages from the found list
	// any packages remaining will be locally installed packages
	// that weren't acquired from a repository
	for (PackageInfoMap::iterator it = remotePackages.begin();
			it != remotePackages.end(); it++) {
		foundPackages.erase(it->first);
	}

	if (!foundPackages.empty()) {
		BString repoName = B_TRANSLATE("Local");
		depots[repoName] = DepotInfo(repoName);
		DepotInfoMap::iterator depot = depots.find(repoName);
		for (PackageInfoMap::iterator it = foundPackages.begin();
				it != foundPackages.end(); ++it) {
			depot->second.AddPackage(it->second);
		}
	}

	for (DepotInfoMap::iterator it = depots.begin(); it != depots.end(); it++) {
		if (fModel.HasDepot(it->second.Name()))
			fModel.SyncDepot(it->second);
		else
			fModel.AddDepot(it->second);
	}

	// start retrieving package icons and average ratings
	if (force || wasEmpty)
		fModel.PopulateAllPackages();

	// compute the OS package dependencies
	try {
		// create the solver
		BSolver* solver;
		status_t error = BSolver::Create(solver);
		if (error != B_OK)
			throw BFatalErrorException(error, "Failed to create solver.");

		ObjectDeleter<BSolver> solverDeleter(solver);
		BPath systemPath;
		error = find_directory(B_SYSTEM_PACKAGES_DIRECTORY, &systemPath);
		if (error != B_OK) {
			throw BFatalErrorException(error,
				"Unable to retrieve system packages directory.");
		}

		// add the "installed" repository with the given packages
		BSolverRepository installedRepository;
		{
			BRepositoryBuilder installedRepositoryBuilder(installedRepository,
				"installed");
			for (int32 i = 0; i < systemFlaggedPackages.CountStrings(); i++) {
				BPath packagePath(systemPath);
				packagePath.Append(systemFlaggedPackages.StringAt(i));
				installedRepositoryBuilder.AddPackage(packagePath.Path());
			}
			installedRepositoryBuilder.AddToSolver(solver, true);
		}

		// add system repository
		BSolverRepository systemRepository;
		{
			BRepositoryBuilder systemRepositoryBuilder(systemRepository,
				"system");
			for (PackageInfoMap::iterator it = systemInstalledPackages.begin();
					it != systemInstalledPackages.end(); it++) {
				BPath packagePath(systemPath);
				packagePath.Append(it->first);
				systemRepositoryBuilder.AddPackage(packagePath.Path());
			}
			systemRepositoryBuilder.AddToSolver(solver, false);
		}

		// solve
		error = solver->VerifyInstallation();
		if (error != B_OK) {
			throw BFatalErrorException(error, "Failed to compute packages to "
				"install.");
		}

		BSolverResult solverResult;
		error = solver->GetResult(solverResult);
		if (error != B_OK) {
			throw BFatalErrorException(error, "Failed to retrieve system "
				"package dependency list.");
		}

		for (int32 i = 0; const BSolverResultElement* element
				= solverResult.ElementAt(i); i++) {
			BSolverPackage* package = element->Package();
			if (element->Type() == BSolverResultElement::B_TYPE_INSTALL) {
				PackageInfoMap::iterator it = systemInstalledPackages.find(
					package->Info().FileName());
				if (it != systemInstalledPackages.end())
					it->second->SetSystemDependency(true);
			}
		}
	} catch (BFatalErrorException ex) {
		printf("Fatal exception occurred while resolving system dependencies: "
			"%s, details: %s\n", strerror(ex.Error()), ex.Details().String());
	} catch (BNothingToDoException) {
		// do nothing
	} catch (BException ex) {
		printf("Exception occurred while resolving system dependencies: %s\n",
			ex.Message().String());
	} catch (...) {
		printf("Unknown exception occurred while resolving system "
			"dependencies.\n");
	}
}


void
MainWindow::_StartRefreshWorker(bool force)
{
	if (fModelWorker != B_BAD_THREAD_ID)
		return;

	RefreshWorkerParameters* parameters = new(std::nothrow)
		RefreshWorkerParameters(this, force);
	if (parameters == NULL)
		return;

	ObjectDeleter<RefreshWorkerParameters> deleter(parameters);
	fModelWorker = spawn_thread(&_RefreshModelThreadWorker, "model loader",
		B_LOW_PRIORITY, parameters);

	if (fModelWorker > 0) {
		deleter.Detach();
		resume_thread(fModelWorker);
	}
}


status_t
MainWindow::_RefreshModelThreadWorker(void* arg)
{
	RefreshWorkerParameters* parameters
		= reinterpret_cast<RefreshWorkerParameters*>(arg);
	MainWindow* mainWindow = parameters->window;
	ObjectDeleter<RefreshWorkerParameters> deleter(parameters);

	BMessenger messenger(mainWindow);

	mainWindow->_RefreshRepositories(parameters->forceRefresh);

	if (mainWindow->fTerminating)
		return B_OK;

	mainWindow->_RefreshPackageList(parameters->forceRefresh);

	messenger.SendMessage(MSG_MODEL_WORKER_DONE);

	return B_OK;
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

		ref->Perform();
	}

	return 0;
}


status_t
MainWindow::_PopulatePackageWorker(void* arg)
{
	MainWindow* window = reinterpret_cast<MainWindow*>(arg);

	while (acquire_sem(window->fPackageToPopulateSem) == B_OK) {
		PackageInfoRef package;
		{
			AutoLocker<BLocker> lock(&window->fPackageToPopulateLock);
			package = window->fPackageToPopulate;
		}

		if (package.Get() != NULL) {
			window->fModel.PopulatePackage(package,
				Model::POPULATE_USER_RATINGS | Model::POPULATE_SCREEN_SHOTS
					| Model::POPULATE_CHANGELOG);
		}
	}

	return 0;
}


void
MainWindow::_NotifyUser(const char* title, const char* message)
{
	BAlert* alert = new(std::nothrow) BAlert(title, message,
		B_TRANSLATE("Close"));

	if (alert != NULL)
		alert->Go();
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
MainWindow::_RatePackage()
{
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
