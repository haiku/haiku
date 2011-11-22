/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUMessagesData.h"

#include <langinfo.h>
#include <string.h>


namespace BPrivate {
namespace Libroot {


ICUMessagesData::ICUMessagesData(pthread_key_t tlsKey)
	: inherited(tlsKey)
{
}


void
ICUMessagesData::Initialize(LocaleMessagesDataBridge* dataBridge)
{
	fPosixLanginfo = dataBridge->posixLanginfo;
}


status_t
ICUMessagesData::SetTo(const Locale& locale, const char* posixLocaleName)
{
	status_t result = inherited::SetTo(locale, posixLocaleName);

	// TODO: open the "POSIX" catalog and fetch the translations from there!

	return result;
}


status_t
ICUMessagesData::SetToPosix()
{
	status_t result = inherited::SetToPosix();

	strcpy(fYesExpression, fPosixLanginfo[YESEXPR]);
	strcpy(fNoExpression, fPosixLanginfo[NOEXPR]);

	return result;
}


const char*
ICUMessagesData::GetLanginfo(int index)
{
	switch(index) {
		case YESEXPR:
			return fYesExpression;
		case NOEXPR:
			return fNoExpression;
		default:
			return "";
	}
}


}	// namespace Libroot
}	// namespace BPrivate
