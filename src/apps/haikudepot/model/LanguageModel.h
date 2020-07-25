/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_MODEL_H
#define LANGUAGE_MODEL_H

#include <vector>

#include <Referenceable.h>

#include "PackageInfo.h"


typedef BReference<Language> LanguageRef;


class LanguageModel {
public:
								LanguageModel();
	virtual						~LanguageModel();

	const	int32				CountSupportedLanguages() const;
	const	LanguageRef			SupportedLanguageAt(int32 index) const;
			void				AddSupportedLanguage(const LanguageRef& value);
			int32				IndexOfSupportedLanguage(
									const BString& languageCode) const;
			void				SetPreferredLanguageToSystemDefault();

	const	LanguageRef			PreferredLanguage() const
									{ return fPreferredLanguage; }

private:
	static	Language			_DeriveSystemDefaultLanguage();
			Language			_DeriveDefaultLanguage() const;
			Language*			_FindSupportedLanguage(
									const BString& code) const;
			void				_SetPreferredLanguage(const Language& language);

private:
			std::vector<LanguageRef>
								fSupportedLanguages;
			LanguageRef			fPreferredLanguage;
};


#endif // LANGUAGE_MODEL_H