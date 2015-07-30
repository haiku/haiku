/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WEB_APP_INTERFACE_H
#define WEB_APP_INTERFACE_H


#include <Application.h>
#include <String.h>
#include <package/PackageVersion.h>

#include "List.h"


class BDataIO;
class BMessage;
using BPackageKit::BPackageVersion;

typedef List<BString, false>	StringList;


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

	static	status_t			SetBaseUrl(const BString& url);
			void				SetPreferredLanguage(const BString& language);
			void				SetArchitecture(const BString& architecture);

			status_t			RetrievePackageInfo(
									const BString& packageName,
									const BString& architecture,
									BMessage& message);

			status_t			RetrieveBulkPackageInfo(
									const StringList& packageNames,
									const StringList& packageArchitectures,
									BMessage& message);

			status_t			RetrievePackageIcon(
									const BString& packageName,
									BDataIO* stream);

			status_t			RetrieveUserRatings(
									const BString& packageName,
									const BString& architecture,
									int resultOffset, int maxResults,
									BMessage& message);

			status_t			RetrieveUserRating(
									const BString& packageName,
									const BPackageVersion& version,
									const BString& architecture,
									const BString& username,
									BMessage& message);

			status_t			CreateUserRating(
									const BString& packageName,
									const BString& architecture,
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

private:
	static  const BString		_GetUserAgentVersionString();
	static	const BString		_GetUserAgent();
			BString				_FormFullUrl(const BString& suffix) const;
			status_t			_SendJsonRequest(const char* domain,
									BString jsonString, uint32 flags,
									BMessage& reply) const;

private:
	static	BString				fBaseUrl;
	static	BString				fUserAgent;
	static	BLocker				fUserAgentLocker;
			BString				fUsername;
			BString				fPassword;
			BString				fLanguage;
	static	int					fRequestIndex;
};


#endif // WEB_APP_INTERFACE_H
