/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WEB_APP_INTERFACE_H
#define WEB_APP_INTERFACE_H


#include <Application.h>
#include <JsonWriter.h>
#include <String.h>
#include <package/PackageVersion.h>

#include "List.h"


class BDataIO;
class BMessage;
using BPackageKit::BPackageVersion;

typedef List<BString, false>	StringList;


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


class WebAppInterface {
public:
								WebAppInterface();
								WebAppInterface(const WebAppInterface& other);
	virtual						~WebAppInterface();

			WebAppInterface&	operator=(const WebAppInterface& other);

			void				SetAuthorization(const BString& username,
									const BString& password);
			const BString&		Username() const
									{ return fUsername; }

			void				SetPreferredLanguage(const BString& language);
			void				SetArchitecture(const BString& architecture);

			status_t			GetChangelog(
									const BString& packageName,
									BMessage& message);

			status_t			RetrieveUserRatings(
									const BString& packageName,
									const BString& architecture,
									int resultOffset, int maxResults,
									BMessage& message);

			status_t			RetrieveUserRating(
									const BString& packageName,
									const BPackageVersion& version,
									const BString& architecture,
									const BString& repositoryCode,
									const BString& username,
									BMessage& message);

			status_t			CreateUserRating(
									const BString& packageName,
									const BPackageVersion& version,
									const BString& architecture,
									const BString& repositoryCode,
									const BString& languageCode,
									const BString& comment,
									const BString& stability,
									int rating,
									BMessage& message);

			status_t			UpdateUserRating(
									const BString& ratingID,
									const BString& languageCode,
									const BString& comment,
									const BString& stability,
									int rating, bool active,
									BMessage& message);

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
									const BString& languageCode,
									BMessage& message);

			status_t			AuthenticateUser(const BString& nickName,
									const BString& passwordClear,
									BMessage& message);

	static int32				ErrorCodeFromResponse(BMessage& response);

private:
			void				_WriteStandardJsonRpcEnvelopeValues(
									BJsonWriter& writer,
									const char* methodName);
			status_t			_SendJsonRequest(const char* domain,
									const BString& jsonString, uint32 flags,
									BMessage& reply) const;
			status_t			_SendJsonRequest(const char* domain,
									BPositionIO* requestData,
									size_t requestDataSize, uint32 flags,
									BMessage& reply) const;
	static	void				_LogPayload(BPositionIO* requestData,
									size_t size);
	static	off_t				_LengthAndSeekToZero(BPositionIO* data);

private:
			BString				fUsername;
			BString				fPassword;
			BString				fLanguage;
	static	int					fRequestIndex;
};


#endif // WEB_APP_INTERFACE_H
