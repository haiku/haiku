/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
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
								const LanguageModel* languagesModel,
								BMenu* menu);
	static	void			MarkLanguageInMenu(
								const BString& languageCode,
								BMenu* menu);

private:
	static	int32			_AddLanguagesToMenu(
								const LanguageModel* languagesModel,
								BMenu* menu, bool isPopular);
	static	void			_AddLanguageToMenu(
								const LanguageRef& language,
								BMenu* menu);
	static	void			_AddLanguageToMenu(
								const BString& code,
								const BString& name, BMenu* menu);
};


#endif // LANGUAGE_MENU_UTILS_H
