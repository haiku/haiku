/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@tycho.email>
 */


#include "UpdateAction.h"

#include <Application.h>
#include <Catalog.h>
#include <package/manager/Exceptions.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UpdateAction"


using namespace BPackageKit;
//using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;


UpdateAction::UpdateAction(bool verbose)
	:
	fVerbose(verbose)
{
	fUpdateManager = new(std::nothrow)
		UpdateManager(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, verbose);
}


UpdateAction::~UpdateAction()
{
	delete fUpdateManager;
}


status_t
UpdateAction::Perform(update_type action_request)
{
	try {
		fUpdateManager->CheckNetworkConnection();
		
		update_type action = action_request;
		// Prompt the user if needed
		if (action == USER_SELECTION_NEEDED)
			action = fUpdateManager->GetUpdateType();
		
		if (action == CANCEL_UPDATE)
			throw BAbortedByUserException();
		else if (action <= INVALID_SELECTION || action >= UPDATE_TYPE_END)
			throw BException(B_TRANSLATE(
				"Invalid update type, cannot continue with updates"));
		
		fUpdateManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
			| BPackageManager::B_ADD_REMOTE_REPOSITORIES
			| BPackageManager::B_REFRESH_REPOSITORIES);
		fUpdateManager->CheckRepositories();
		
//		fUpdateManager->SetDebugLevel(1);
		if(action == UPDATE) {
			// These values indicate that all updates should be installed
			int packageCount = 0;
			const char* const packages = "";
			fUpdateManager->Update(&packages, packageCount);
		} else if (action == FULLSYNC)
			fUpdateManager->FullSync();
		else
			// Should not happen but just in case
			throw BException(B_TRANSLATE(
				"Invalid update type, cannot continue with updates"));
	
	} catch (BFatalErrorException ex) {
		fUpdateManager->FinalUpdate(B_TRANSLATE("Updates did not complete"),
			ex.Message());
		return ex.Error();
	} catch (BAbortedByUserException ex) {
		if (fVerbose)
			fprintf(stderr, "Updates aborted by user: %s\n",
				ex.Message().String());
		// No need for a final message since user initiated cancel request
		be_app->PostMessage(kMsgFinalQuit);
		return B_OK;
	} catch (BNothingToDoException ex) {
		if (fVerbose)
			fprintf(stderr, "Nothing to do while updating packages : %s\n",
				ex.Message().String());
		fUpdateManager->FinalUpdate(B_TRANSLATE("No updates available"),
			B_TRANSLATE("There were no updates found."));
		return B_OK;
	} catch (BException ex) {
		if (fVerbose)
			fprintf(stderr, B_TRANSLATE(
				"Exception occurred while updating packages : %s\n"),
				ex.Message().String());
		fUpdateManager->FinalUpdate(B_TRANSLATE("Updates did not complete"),
			ex.Message());
		return B_ERROR;
	}
	
	return B_OK;
}
