/*
 * Copyright 2018-2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef APP_UTILS_H
#define APP_UTILS_H


#include "Alert.h"
#include "Menu.h"


class AppUtils {

public:
	static	void			NotifySimpleError(const char* title,
								const char* text,
								alert_type type = B_INFO_ALERT);

	static	status_t		MarkItemWithCodeInMenuOrFirst(const BString& code,
								BMenu* menu);
	static	status_t		MarkItemWithCodeInMenu(const BString& code,
								BMenu* menu);
	static	int32			IndexOfCodeInMenu(const BString& code, BMenu* menu);
	static	status_t		GetCodeAtIndexInMenu(BMenu* menu, int32 index,
								BString* result);

	static	status_t		GetAppVersionString(BString& result);
};


#endif // APP_UTILS_H
