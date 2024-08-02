/*
 * Copyright 2019-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_MENU_UTILS_H
#define LANGUAGE_MENU_UTILS_H


#include <vector>

#include <Menu.h>

#include "Model.h"
#include "PackageInfo.h"


class LanguageMenuUtils {

public:
	static	void			AddLanguagesToMenu(
								const LanguageRepository* repository,
								BMenu* menu);
	static	void			MarkLanguageInMenu(
								const BString& languageId,
								BMenu* menu);

private:
	static	int32			_AddLanguagesToMenu(
								const std::vector<LanguageRef>& languages,
								BMenu* menu, bool isPopular);
	static	void			_AddLanguageToMenu(
								const LanguageRef& language,
								BMenu* menu);
	static	void			_AddLanguageToMenu(
								const BString& code,
								const BString& name, BMenu* menu);

	static	int				_LanguagesPresentationCompareFn(const LanguageRef& l1,
								const LanguageRef& l2);
	static	bool			_IsLanguagePresentationBefore(const LanguageRef& l1,
								const LanguageRef& l2);
};


#endif // LANGUAGE_MENU_UTILS_H
