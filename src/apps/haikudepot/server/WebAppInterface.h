/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2017, Andrew Lindesay <apl@lindesay.co.nz>.
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

			void				SetPreferredLanguage(const BString& language);
			void				SetArchitecture(const BString& architecture);

			status_t			RetrieveRepositoriesForSourceBaseURLs(
									const StringList& repositorySourceBaseURL,
									BMessage& message);

			status_t			RetrievePackageInfo(
									const BString& packageName,
									const BString& architecture,
									const BString& repositoryCode,
									BMessage& message);

			status_t			RetrieveBulkPackageInfo(
									const StringList& packageNames,
									const StringList& packageArchitectures,
									const StringList& repositoryCodes,
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

private:
			status_t			_SendJsonRequest(const char* domain,
									BString jsonString, uint32 flags,
									BMessage& reply) const;

private:
			BString				fUsername;
			BString				fPassword;
			BString				fLanguage;
	static	int					fRequestIndex;
};


#endif // WEB_APP_INTERFACE_H
