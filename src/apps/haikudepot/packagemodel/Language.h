/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_H
#define LANGUAGE_H


#include <Language.h>
#include <Referenceable.h>


/*! This class represents a language that is supported by the Haiku
    Depot Server system.  This may differ from the set of languages
    that are supported in the platform itself.

    No builder is provided for this class because it sub-classes from
    a Haiku type. Instead always create new instances with the constructor
    and do not use any mutating functions offered by the superclass.
*/

class Language : public BReferenceable, public BLanguage {
public:
								Language(const char* language,
									const BString& serverName,
									bool isPopular);
								Language(const Language& other);

			bool				operator<(const Language& other) const;
			bool				operator==(const Language& other) const;
			bool				operator!=(const Language& other) const;

			status_t			GetName(BString& name,
									const BLanguage* displayLanguage = NULL
									) const;
			bool				IsPopular() const
									{ return fIsPopular; }

			int					Compare(const Language& language) const;

private:
			BString				fServerName;
			bool				fIsPopular;
};


typedef BReference<Language> LanguageRef;


extern bool IsLanguageRefLess(const LanguageRef& l1, const LanguageRef& l2);


#endif // LANGUAGE_H
