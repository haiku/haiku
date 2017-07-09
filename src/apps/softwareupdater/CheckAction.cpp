/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@tycho.email>
 */


#include "CheckAction.h"

#include <Application.h>
#include <Catalog.h>
#include <package/manager/Exceptions.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CheckAction"


using namespace BPackageKit;
//using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;


CheckAction::CheckAction(bool verbose)
{
	fCheckManager = new(std::nothrow)
		CheckManager(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, verbose);
}


CheckAction::~CheckAction()
{
	delete fCheckManager;
}


status_t
CheckAction::Perform()
{
	try {
		fCheckManager->CheckNetworkConnection();
		
		fCheckManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
			| BPackageManager::B_ADD_REMOTE_REPOSITORIES
			| BPackageManager::B_REFRESH_REPOSITORIES);
		
//		fUpdateManager->SetDebugLevel(1);
		// These values indicate that all updates should be installed
		int packageCount = 0;
		const char* const packages = "";
		fCheckManager->Update(&packages, packageCount);
	} catch (BFatalErrorException ex) {
		fprintf(stderr, B_TRANSLATE(
				"Fatal error while checking for updates: %s\n"),
			ex.Message().String());
		be_app->PostMessage(kMsgFinalQuit);
		return ex.Error();
	} catch (BAbortedByUserException ex) {
		be_app->PostMessage(kMsgFinalQuit);
		return B_OK;
	} catch (BNothingToDoException ex) {
		puts(B_TRANSLATE("There were no updates found."));
		fCheckManager->NoUpdatesNotification();
		be_app->PostMessage(kMsgFinalQuit);
		return B_OK;
	} catch (BException ex) {
		fprintf(stderr, B_TRANSLATE(
				"Exception occurred while checking for updates: %s\n"),
			ex.Message().String());
		be_app->PostMessage(kMsgFinalQuit);
		return B_ERROR;
	}
	
	return B_OK;
}
