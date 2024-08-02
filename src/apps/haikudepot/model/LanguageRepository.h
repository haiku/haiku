/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_REPOSITORY_H
#define LANGUAGE_REPOSITORY_H

#include <vector>

#include "Language.h"


class LanguageRepository
{
public:
								LanguageRepository();
	virtual						~LanguageRepository();

			void				Clear();
			bool				IsEmpty() const;
			int32				CountLanguages() const;
	const	LanguageRef			LanguageAtIndex(int32 index) const;
			void				AddLanguage(const LanguageRef& value);

			int32				IndexOfLanguage(const char* code, const char* countryCode,
									const char* scriptCode) const;

private:
			std::vector<LanguageRef>
								fLanguages;
};


#endif // LANGUAGE_REPOSITORY_H
