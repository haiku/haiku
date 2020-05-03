/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_MODEL_H
#define LANGUAGE_MODEL_H


#include <Referenceable.h>

#include "List.h"
#include "PackageInfo.h"


typedef BReference<Language> LanguageRef;
typedef List<LanguageRef, false> LanguageList;


class LanguageModel {
public:
								LanguageModel();
	virtual						~LanguageModel();

			const LanguageList&	SupportedLanguages() const
									{ return fSupportedLanguages; }
			void				AddSupportedLanguages(
									const LanguageList& languages);
			int32				IndexOfSupportedLanguage(
									const BString& languageCode) const;

			const LanguageRef	PreferredLanguage() const
									{ return fPreferredLanguage; }

private:
	static	Language			_DeriveSystemDefaultLanguage();
			Language			_DeriveDefaultLanguage() const;
			Language*			_FindSupportedLanguage(
									const BString& code) const;
			void				_SetPreferredLanguage(const Language& language);

private:
			LanguageList		fSupportedLanguages;
			LanguageRef			fPreferredLanguage;
};


#endif // LANGUAGE_MODEL_H