/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_CATEGORY_DATA_H
#define _ICU_CATEGORY_DATA_H

#include <pthread.h>

#include <unicode/locid.h>
#include <unicode/unistr.h>

#include <SupportDefs.h>

#include "ICUThreadLocalStorageValue.h"


namespace BPrivate {
namespace Libroot {


class ICUCategoryData {
public:
								ICUCategoryData(pthread_key_t tlsKey);
	virtual 					~ICUCategoryData();

	virtual	status_t			SetTo(const Locale& locale,
									const char* posixLocaleName);
	virtual	status_t			SetToPosix();

			const char*			PosixLocaleName()
									{ return fPosixLocaleName; }

			const Locale&		ICULocale() const;

protected:
			status_t			_ConvertUnicodeStringToLocaleconvEntry(
									const UnicodeString& string,
									char* destination, int destinationSize,
									const char* defaultValue = "");

			status_t			_GetConverter(UConverter*& converterOut);

	static	const uint16		skMaxPosixLocaleNameLen = 128;
	static	const size_t		skLCBufSize = 16;

			pthread_key_t		fThreadLocalStorageKey;
			Locale				fLocale;
			char				fPosixLocaleName[skMaxPosixLocaleNameLen];
			char				fGivenCharset[UCNV_MAX_CONVERTER_NAME_LENGTH];

private:
 			status_t			_SetupConverter();
};


inline
const Locale&
ICUCategoryData::ICULocale() const
{
	return fLocale;
}


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _ICU_CATEGORY_DATA_H
