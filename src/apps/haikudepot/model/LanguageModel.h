/*
 * Copyright 2019-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_MODEL_H
#define LANGUAGE_MODEL_H

#include <vector>

#include <Referenceable.h>

#include "Language.h"


typedef BReference<Language> LanguageRef;


class LanguageModel {
public:
								LanguageModel();
								LanguageModel(BString forcedSystemDefaultLanguage);
	virtual						~LanguageModel();

			void				ClearSupportedLanguages();
	const	int32				CountSupportedLanguages() const;
	const	LanguageRef			SupportedLanguageAt(int32 index) const;
			void				AddSupportedLanguage(const LanguageRef& value);
			void				SetPreferredLanguageToSystemDefault();

	const	LanguageRef			PreferredLanguage() const
									{ return fPreferredLanguage; }

private:
			int32				_IndexOfSupportedLanguage(
									const char* code,
									const char* countryCode,
									const char* scriptCode) const;
			int32				_IndexOfBestMatchingSupportedLanguage(
									const char* code,
									const char* countryCode,
									const char* scriptCode) const;

			Language			_DeriveSystemDefaultLanguage() const;
			Language			_DeriveDefaultLanguage() const;
			Language*			_FindBestSupportedLanguage(
									const char* code,
            						const char* countryCode,
            						const char* scriptCode) const;
			void				_SetPreferredLanguage(const Language& language);

	static	int					_NullSafeStrCmp(const char* s1, const char* s2);

	static	int					_LanguagesCompareFn(const LanguageRef& l1, const LanguageRef& l2);
	static	bool				_IsLanguageBefore(const LanguageRef& l1, const LanguageRef& l2);

private:
			std::vector<LanguageRef>
								fSupportedLanguages;
			LanguageRef			fPreferredLanguage;
			BString				fForcedSystemDefaultLanguage;
};


#endif // LANGUAGE_MODEL_H