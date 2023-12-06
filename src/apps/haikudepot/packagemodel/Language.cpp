/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


 #include "Language.h"


 Language::Language(const char* language, const BString& serverName,
 	bool isPopular)
 	:
 	BLanguage(language),
 	fServerName(serverName),
 	fIsPopular(isPopular)
 {
 }


 Language::Language(const Language& other)
 	:
 	BLanguage(other.Code()),
 	fServerName(other.fServerName),
 	fIsPopular(other.fIsPopular)
 {
 }


 status_t
 Language::GetName(BString& name,
 	const BLanguage* displayLanguage) const
 {
 	status_t result = BLanguage::GetName(name, displayLanguage);

 	if (result == B_OK && (name.IsEmpty() || name == Code()))
 		name.SetTo(fServerName);

 	return result;
 }
