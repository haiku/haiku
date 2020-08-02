/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICULocaleconvData.h"

#include <string.h>


U_NAMESPACE_USE


namespace BPrivate {
namespace Libroot {


ICULocaleconvData::ICULocaleconvData(pthread_key_t tlsKey)
	: inherited(tlsKey)
{
}


status_t
ICULocaleconvData::_SetLocaleconvEntry(const DecimalFormatSymbols* formatSymbols,
	char* destination, FormatSymbol symbol, const char* defaultValue)
{
	status_t result = B_OK;

	UnicodeString symbolString = formatSymbols->getSymbol(symbol);
	if (!symbolString.isEmpty()) {
		result = _ConvertUnicodeStringToLocaleconvEntry(symbolString,
			destination, skLCBufSize, defaultValue);
	} else
		destination[0] = '\0';

	return result;
}


}	// namespace Libroot
}	// namespace BPrivate
