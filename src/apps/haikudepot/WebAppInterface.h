/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WEB_APP_INTERFACE_H
#define WEB_APP_INTERFACE_H


#include <Application.h>
#include <String.h>


class BDataIO;
class BMessage;


class WebAppInterface {
public:
								WebAppInterface();
	virtual						~WebAppInterface();

			void				SetAuthorization(const BString& username,
									const BString& password);

			status_t			RetrievePackageInfo(
									const BString& packageName,
									BMessage& message);

			status_t			RetrievePackageIcon(
									const BString& packageName,
									BDataIO* stream);

private:
			BString				fUsername;
			BString				fPassword;
	static	int					fRequestIndex;
};


#endif // WEB_APP_INTERFACE_H
