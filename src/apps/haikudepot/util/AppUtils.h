/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef APP_UTILS_H
#define APP_UTILS_H


#include "Menu.h"


class AppUtils {

public:
	static	void			NotifySimpleError(const char* title,
								const char* text);

	static	status_t		MarkItemWithCodeInMenuOrFirst(const BString& code,
								BMenu* menu);
	static	status_t		MarkItemWithCodeInMenu(const BString& code,
								BMenu* menu);
	static	int32			IndexOfCodeInMenu(const BString& code, BMenu* menu);
	static	status_t		GetCodeAtIndexInMenu(BMenu* menu, int32 index,
								BString* result);
};


#endif // APP_UTILS_H
