/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UserDetailVerifierProcess.h"

#include <AutoLocker.h>
#include <Catalog.h>
#include <Window.h>

#include "AppUtils.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerHelper.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UserDetailVerifierProcess"


UserDetailVerifierProcess::UserDetailVerifierProcess(Model* model,
	UserDetailVerifierListener* listener)
	:
	fModel(model),
	fListener(listener)
{
}


UserDetailVerifierProcess::~UserDetailVerifierProcess()
{
}


const char*
UserDetailVerifierProcess::Name() const
{
	return "UserDetailVerifierProcess";
}


const char*
UserDetailVerifierProcess::Description() const
{
	return B_TRANSLATE("Checking user details");
}


status_t
UserDetailVerifierProcess::RunInternal()
{
	status_t result = B_OK;

	if (_ShouldVerify()) {
		UserDetail userDetail;
		result = _TryFetchUserDetail(userDetail);

		switch (result) {
			case B_PERMISSION_DENIED:
				fListener->UserCredentialsFailed();
				result = B_OK;
				break;
			case B_OK:
				if (!userDetail.Agreement().IsLatest()) {
					HDINFO("the user has not agreed to the latest user usage"
						" conditions.");
					fListener->UserUsageConditionsNotLatest(userDetail);
				}
				break;
			default:
				break;
		}
	}

	return result;
}


bool
UserDetailVerifierProcess::_ShouldVerify()
{
	if (!ServerHelper::IsNetworkAvailable()) {
		HDINFO("no network --> will not verify user");
		return false;
	}

	{
		AutoLocker<BLocker> locker(fModel->Lock());
		if (fModel->Nickname().IsEmpty()) {
			HDINFO("no nickname --> will not verify user");
			return false;
		}
	}

	return true;
}


status_t
UserDetailVerifierProcess::_TryFetchUserDetail(UserDetail& userDetail)
{
	WebAppInterface interface = fModel->GetWebAppInterface();
	BMessage userDetailResponse;
	status_t result;

	result = interface.RetrieveCurrentUserDetail(userDetailResponse);
	if (result != B_OK) {
		HDERROR("a problem has arisen retrieving the current user detail: %s",
			strerror(result));
	}

	if (result == B_OK) {
		int32 errorCode = interface.ErrorCodeFromResponse(userDetailResponse);
		switch (errorCode) {
			case ERROR_CODE_NONE:
				break;
			case ERROR_CODE_AUTHORIZATIONFAILURE:
				result = B_PERMISSION_DENIED;
				break;
			default:
				HDERROR("a problem has arisen retrieving the current user "
					"detail for user [%s]: jrpc error code %" B_PRId32 "",
					fModel->Nickname().String(), errorCode);
				result = B_ERROR;
				break;
		}
	}

	if (result == B_OK) {

		// now we have the user details by showing that an authentication has
		// worked, it is now necessary to check to see that the user has agreed
		// to the most recent user-usage conditions.

		result = interface.UnpackUserDetail(userDetailResponse, userDetail);
		if (result != B_OK)
			HDERROR("it was not possible to unpack the user details.");
	}

	return result;
}