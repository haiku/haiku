/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_LOCALECONV_DATA_H
#define _ICU_LOCALECONV_DATA_H


#include "ICUCategoryData.h"

#include <unicode/decimfmt.h>


namespace BPrivate {
namespace Libroot {


typedef U_NAMESPACE_QUALIFIER DecimalFormatSymbols::ENumberFormatSymbol
	FormatSymbol;

class ICULocaleconvData : public ICUCategoryData {
	typedef	ICUCategoryData		inherited;

protected:
								ICULocaleconvData(pthread_key_t tlsKey);

			status_t			_SetLocaleconvEntry(
									const U_NAMESPACE_QUALIFIER
										DecimalFormatSymbols* formatSymbols,
									char* destination, FormatSymbol symbol,
									const char* defaultValue = "");
};


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _ICU_LOCALECONV_DATA_H
