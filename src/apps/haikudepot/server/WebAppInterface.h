/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WEB_APP_INTERFACE_H
#define WEB_APP_INTERFACE_H


#include <Application.h>
#include <JsonWriter.h>
#include <Locker.h>
#include <Referenceable.h>
#include <String.h>
#include <package/PackageVersion.h>

#include "AccessToken.h"
#include "DepotInfo.h"
#include "PackageInfo.h"
#include "PasswordRequirements.h"
#include "UserCredentials.h"
#include "UserDetail.h"
#include "UserUsageConditions.h"


class BDataIO;
class BMessage;
using BPackageKit::BPackageVersion;


/*! These are error codes that are sent back to the client from the server */

#define ERROR_CODE_NONE							0
#define ERROR_CODE_VALIDATION					-32800
#define ERROR_CODE_OBJECTNOTFOUND				-32801
#define ERROR_CODE_CAPTCHABADRESPONSE			-32802
#define ERROR_CODE_AUTHORIZATIONFAILURE			-32803
#define ERROR_CODE_BADPKGICON					-32804
#define ERROR_CODE_LIMITEXCEEDED				-32805
#define ERROR_CODE_AUTHORIZATIONRULECONFLICT	-32806

/*! This constant can be used to indicate the lack of a rating. */

#define RATING_NONE -1


class WebAppInterface : public BReferenceable
{
public:
								WebAppInterface(const UserCredentials& value);
	virtual						~WebAppInterface();

			const BString&		Nickname();

			status_t			GetChangelog(
									const BString& packageName,
									BMessage& message);

			status_t			RetrieveUserRatingSummaryForPackage(
									const BString& packageName,
                                    const BString& webAppRepositoryCode,
                                    BMessage& message);

			status_t			RetrieveUserRatingsForPackageForDisplay(
									const BString& packageName,
									const BString& webAppRepositoryCode,
									const BString& webAppRepositorySourceCode,
									int resultOffset, int maxResults,
									BMessage& message);

			status_t			RetrieveUserRatingForPackageAndVersionByUser(
									const BString& packageName,
									const BPackageVersion& version,
									const BString& architecture,
									const BString& webAppRepositoryCode,
									const BString& webAppRepositorySourceCode,
									const BString& userNickname,
									BMessage& message);

			status_t			CreateUserRating(
									const BString& packageName,
									const BPackageVersion& version,
									const BString& architecture,
									const BString& webAppRepositoryCode,
									const BString& webAppRepositorySourceCode,
									const BString& naturalLanguageCode,
										// This is the "ID" in the ICU system; the term `code`
										// is used with the server system.
									const BString& comment,
									const BString& stability,
									int rating,
									BMessage& message);

			status_t			UpdateUserRating(
									const BString& ratingID,
									const BString& naturalLanguageCode,
										// This is the "ID" in the ICU system; the term `code`
										// is used with the server system.
									const BString& comment,
									const BString& stability,
									int rating, bool active,
									BMessage& message);

			status_t			RetrieveUserDetailForCredentials(
									const UserCredentials& credentials,
									BMessage& message);

			status_t			RetrieveCurrentUserDetail(
									BMessage& message);

			status_t			RetrieveUserUsageConditions(
									const BString& code,
									UserUsageConditions& conditions);

			status_t			AgreeUserUsageConditions(const BString& code,
									BMessage& responsePayload);

			status_t			RetrieveScreenshot(
									const BString& code,
									int32 width, int32 height,
									BDataIO* stream);

			status_t			RequestCaptcha(BMessage& message);

			status_t			CreateUser(const BString& nickName,
									const BString& passwordClear,
									const BString& email,
									const BString& captchaToken,
									const BString& captchaResponse,
									const BString& naturalLanguageCode,
										// This is the "ID" in the ICU system; the term `code`
										// is used with the server system.
									const BString& userUsageConditionsCode,
									BMessage& message);

			status_t			AuthenticateUserRetainingAccessToken();

			status_t			AuthenticateUser(const BString& nickName,
									const BString& passwordClear,
									BMessage& message);

			status_t			IncrementViewCounter(
									const PackageInfoRef package,
									const DepotInfoRef depot,
									BMessage& message);

			status_t			RetrievePasswordRequirements(
									PasswordRequirements& passwordRequirements);

	static	int32				ErrorCodeFromResponse(
									BMessage& responseEnvelopeMessage);

	static	status_t			UnpackUserDetail(
									BMessage& responseEnvelopeMessage,
									UserDetail& userDetail);

	static	status_t			UnpackAccessToken(
									BMessage& responseEnvelopeMessage,
									AccessToken& accessToken);
private:
			void				_SetCredentials(const UserCredentials& value);
			UserCredentials		_Credentials();

			AccessToken			_ObtainValidAccessToken();

			status_t			_AuthenticateUserRetainingAccessToken(const BString& nickName,
									const BString& passwordClear);

			status_t			_RetrievePasswordRequirementsMeta(
									BMessage& message);

			status_t			_RetrieveUserUsageConditionsMeta(
									const BString& code, BMessage& message);
			status_t			_RetrieveUserUsageConditionsCopy(
									const BString& code, BDataIO* stream);

			status_t			_SendJsonRequest(const char* urlPathComponents,
									const BString& jsonString, uint32 flags,
									BMessage& reply);
			status_t			_SendJsonRequest(const char* urlPathComponents,
									BPositionIO* requestData,
									size_t requestDataSize, uint32 flags,
									BMessage& reply);
	static	status_t			_SendJsonRequest(const char* urlPathComponents,
									const AccessToken& accessToken,
									BPositionIO* requestData,
									size_t requestDataSize, uint32 flags,
									BMessage& reply);

			status_t			_SendRawGetRequest(
									const BString urlPathComponents,
									BDataIO* stream);
	static	void				_LogPayload(BPositionIO* requestData,
									size_t size);
	static	off_t				_LengthAndSeekToZero(BPositionIO* data);

private:
			UserCredentials		fCredentials;
			AccessToken			fAccessToken;
			BLocker				fLock;
};


typedef BReference<WebAppInterface> WebAppInterfaceRef;


#endif // WEB_APP_INTERFACE_H
