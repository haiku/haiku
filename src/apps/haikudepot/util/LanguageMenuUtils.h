/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_MENU_UTILS_H
#define LANGUAGE_MENU_UTILS_H


#include <Menu.h>

#include "Model.h"
#include "PackageInfo.h"


class LanguageMenuUtils {

public:
	static	void			AddLanguagesToMenu(
								const LanguageList& languages,
								BMenu* menu);
	static	void			MarkLanguageInMenu(
								const BString& languageCode,
								BMenu* menu);

private:
	static	int32			_IndexOfLanguageInMenu(
								const BString& languageCode,
								BMenu* menu);
	static	status_t		_GetLanguageAtIndexInMenu(BMenu* menu,
								int32 index, BString* result);
	static	int32			_AddLanguagesToMenu(
								const LanguageList& languages,
								BMenu* menu, bool isPopular);
	static	void			_AddLanguageToMenu(
								const LanguageRef& language,
								BMenu* menu);
	static	void			_AddLanguageToMenu(
								const BString& code,
								const BString& name, BMenu* menu);
};


#endif // LANGUAGE_MENU_UTILS_H
