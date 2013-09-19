/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
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
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <StringList.h>
#include <TabView.h>

#include <package/Context.h>
#include <package/manager/Exceptions.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/PackageRoster.h>
#include "package/RepositoryCache.h"
#include <package/solver/SolverPackage.h>

#include "DecisionProvider.h"
#include "FilterView.h"
#include "JobStateListener.h"
#include "PackageInfoView.h"
#include "PackageListView.h"



#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_MODEL_WORKER_DONE = 'mmwd',
	MSG_REFRESH_DEPOTS = 'mrdp'
};


using namespace BPackageKit;
using BManager::BPrivate::BException;
using BManager::BPrivate::BFatalErrorException;


typedef std::map<BString, PackageInfoRef> PackageInfoMap;
typedef std::map<BString, BObjectList<PackageInfo> > PackageLocationMap;
typedef std::map<BString, DepotInfo> DepotInfoMap;


MainWindow::MainWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("HaikuDepot"),
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fPackageManager(B_PACKAGE_INSTALLATION_LOCATION_HOME),
	fTerminating(false),
	fModelWorker(B_BAD_THREAD_ID)
{
	BMenuBar* menuBar = new BMenuBar(B_TRANSLATE("Main Menu"));
	_BuildMenu(menuBar);

	fFilterView = new FilterView(fModel);
	fPackageListView = new PackageListView(fModel.Lock());
	fPackageInfoView = new PackageInfoView(fModel.Lock(), &fPackageManager);

	fSplitView = new BSplitView(B_VERTICAL, 5.0f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(menuBar)
		.Add(fFilterView)
		.AddSplit(fSplitView)
			.AddGroup(B_VERTICAL)
				.Add(fPackageListView)
				.SetInsets(
					B_USE_DEFAULT_SPACING, 0.0f,
					B_USE_DEFAULT_SPACING, 0.0f)
			.End()
			.Add(fPackageInfoView)
		.End()
	;

	fSplitView->SetCollapsible(0, false);
	fSplitView->SetCollapsible(1, false);

	_StartRefreshWorker();
}


MainWindow::~MainWindow()
{
	fTerminating = true;
	if (fModelWorker > 0) {
		status_t result;
		wait_for_thread(fModelWorker, &result);
	}
}


bool
MainWindow::QuitRequested()
{
	BMessage message(MSG_MAIN_WINDOW_CLOSED);
	message.AddRect("window frame", Frame());
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
			break;
		}
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			// TODO: ?
			break;

		case MSG_REFRESH_DEPOTS:
		{
			_StartRefreshWorker();
			break;
		}
		case MSG_PACKAGE_SELECTED:
		{
			BString title;
			if (message->FindString("title", &title) == B_OK) {
				int count = fVisiblePackages.CountItems();
				for (int i = 0; i < count; i++) {
					const PackageInfoRef& package
						= fVisiblePackages.ItemAtFast(i);
					if (package.Get() != NULL && package->Title() == title) {
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
			fModel.SetCategory(name);
			_AdoptModel();
			break;
		}

		case MSG_DEPOT_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				name = "";
			fModel.SetDepot(name);
			_AdoptModel();
			break;
		}

		case MSG_SEARCH_TERMS_MODIFIED:
		{
			// TODO: Do this with a delay!
			BString searchTerms;
			if (message->FindString("search terms", &searchTerms) != B_OK)
				searchTerms = "";
			fModel.SetSearchTerms(searchTerms);
			_AdoptModel();
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MainWindow::_BuildMenu(BMenuBar* menuBar)
{
	BMenu* menu = new BMenu(B_TRANSLATE("Package"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Refresh depots"),
			new BMessage(MSG_REFRESH_DEPOTS)));
	menuBar->AddItem(menu);

}


void
MainWindow::_AdoptModel()
{
	fVisiblePackages = fModel.CreatePackageList();

	fPackageListView->Clear();
	for (int32 i = 0; i < fVisiblePackages.CountItems(); i++) {
		BAutolock _(fModel.Lock());
		fPackageListView->AddPackage(fVisiblePackages.ItemAtFast(i));
	}
}


void
MainWindow::_AdoptPackage(const PackageInfoRef& package)
{
	fPackageInfoView->SetPackage(package);
	fModel.PopulatePackage(package);
}


void
MainWindow::_ClearPackage()
{
	fPackageInfoView->Clear();
}


void
MainWindow::_RefreshRepositories(bool force)
{
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

		if (roster.GetRepositoryCache(repoName, &cache) != B_OK
			|| force) {
			try {
				BRefreshRepositoryRequest refreshRequest(context, repoConfig);

				result = refreshRequest.Process();
			} catch (BFatalErrorException ex) {
				fprintf(stderr, "Fatal error occurred while refreshing "
					"repository: %s (%s)\n", ex.Message().String(),
					ex.Details().String());
			} catch (BException ex) {
				fprintf(stderr, "Exception occurred while refreshing "
					"repository: %s\n", ex.Message().String());
			}
		}
	}
}


void
MainWindow::_RefreshPackageList()
{
	BPackageRoster roster;
	BStringList repositoryNames;

	status_t result = roster.GetRepositoryNames(repositoryNames);
	if (result != B_OK)
		return;

	DepotInfoMap depots;
	for (int32 i = 0; i < repositoryNames.CountStrings(); i++) {
		const BString& repoName = repositoryNames.StringAt(i);
		depots[repoName] = DepotInfo(repoName);
	}

	try {
		fPackageManager.Init(PackageManager::B_ADD_INSTALLED_REPOSITORIES
			| PackageManager::B_ADD_REMOTE_REPOSITORIES);
	} catch (BException ex) {
		fprintf(stderr, "Exception occurred while initializing PackageManager:"
			"%s\n", ex.Message().String());
		return;
	}

	BObjectList<BSolverPackage> packages;
	result = fPackageManager.Solver()->FindPackages("",
		BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME
			| BSolver::B_FIND_IN_SUMMARY | BSolver::B_FIND_IN_DESCRIPTION
			| BSolver::B_FIND_IN_PROVIDES,
		packages);
	if (result != B_OK) {
		// TODO: notify user
		return;
	}

	if (packages.IsEmpty())
		return;

	PackageLocationMap packageLocations;
	PackageInfoMap foundPackages;
		// if a given package is installed locally, we will potentially
		// get back multiple entries, one for each local installation
		// location, and one for each remote repository the package
		// is available in. The above map is used to ensure that in such
		// cases we consolidate the information, rather than displaying
		// duplicates
	for (int32 i = 0; i < packages.CountItems(); i++) {
		BSolverPackage* package = packages.ItemAt(i);
		const BPackageInfo& repoPackageInfo = package->Info();
		PackageInfoRef modelInfo;
		PackageInfoMap::iterator it = foundPackages.find(
			repoPackageInfo.Name());
		if (it != foundPackages.end())
			modelInfo.SetTo(it->second);
		else {
			BString publisherURL;
			if (repoPackageInfo.URLList().CountStrings() > 0)
				publisherURL = repoPackageInfo.URLList().StringAt(0);
			modelInfo.SetTo(new(std::nothrow) PackageInfo(NULL,
					repoPackageInfo.Name(),
					repoPackageInfo.Version().ToString(),
					PublisherInfo(BitmapRef(), repoPackageInfo.Vendor(),
					"", publisherURL), repoPackageInfo.Summary(),
					repoPackageInfo.Description(), ""),
				true);

			if (modelInfo.Get() == NULL)
				return;

			foundPackages[repoPackageInfo.Name()] = modelInfo;
		}

		BSolverRepository* repository = package->Repository();
		if (dynamic_cast<BPackageManager::RemoteRepository*>(repository)
				!= NULL) {
			depots[repository->Name()].AddPackage(modelInfo);
		} else {
			const char* installationLocation = NULL;
			if (repository == static_cast<const BSolverRepository*>(
					fPackageManager.SystemRepository())) {
				installationLocation = "system";
			} else if (repository == static_cast<const BSolverRepository*>(
					fPackageManager.CommonRepository())) {
				installationLocation = "common";
			} else if (repository == static_cast<const BSolverRepository*>(
					fPackageManager.HomeRepository())) {
				installationLocation = "home";
			}

			if (installationLocation != NULL) {
				packageLocations[installationLocation].AddItem(
					modelInfo.Get());
			}
		}
	}

	BAutolock lock(fModel.Lock());

	fModel.Clear();

	for (DepotInfoMap::iterator it = depots.begin(); it != depots.end();
		++it) {
		fModel.AddDepot(it->second);
	}

	for (PackageLocationMap::iterator it = packageLocations.begin();
		it != packageLocations.end(); ++it) {
		for (int32 i = 0; i < it->second.CountItems(); i++) {
			fModel.SetPackageState(it->second.ItemAt(i), ACTIVATED);
				// TODO: indicate the specific installation location
				// and verify that the package is in fact activated
				// by querying the package roster
		}
	}
}


void
MainWindow::_StartRefreshWorker()
{
	if (fModelWorker != B_BAD_THREAD_ID)
		return;

	fModelWorker = spawn_thread(&_RefreshModelThreadWorker, "model loader",
		B_LOW_PRIORITY, this);

	if (fModelWorker > 0)
		resume_thread(fModelWorker);
}


status_t
MainWindow::_RefreshModelThreadWorker(void* arg)
{
	MainWindow* mainWindow = reinterpret_cast<MainWindow*>(arg);

	BMessenger messenger(mainWindow);

	mainWindow->_RefreshRepositories();

	if (mainWindow->fTerminating)
		return B_OK;

	mainWindow->_RefreshPackageList();

	messenger.SendMessage(MSG_MODEL_WORKER_DONE);

	return B_OK;
}


