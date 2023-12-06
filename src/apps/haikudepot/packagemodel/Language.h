/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_H
#define LANGUAGE_H


#include <Language.h>
#include <Referenceable.h>


/*! This class represents a language that is supported by the Haiku
    Depot Server system.  This may differ from the set of languages
    that are supported in the platform itself.
*/

class Language : public BReferenceable, public BLanguage {
public:
								Language(const char* language,
									const BString& serverName,
									bool isPopular);
								Language(const Language& other);

			status_t			GetName(BString& name,
									const BLanguage* displayLanguage = NULL
									) const;
			bool				IsPopular() const
									{ return fIsPopular; }

private:
			BString				fServerName;
			bool				fIsPopular;
};


#endif // LANGUAGE_H
