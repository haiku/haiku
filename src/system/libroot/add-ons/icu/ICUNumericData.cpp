/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUNumericData.h"

#include <langinfo.h>
#include <locale.h>
#include <strings.h>


namespace BPrivate {
namespace Libroot {


ICUNumericData::ICUNumericData(pthread_key_t tlsKey, struct lconv& localeConv)
	:
	inherited(tlsKey),
	fLocaleConv(localeConv),
	fDataBridge(NULL)
{
	fLocaleConv.decimal_point = fDecimalPoint;
	fLocaleConv.thousands_sep = fThousandsSep;
	fLocaleConv.grouping = fGrouping;
}


void
ICUNumericData::Initialize(LocaleNumericDataBridge* dataBridge)
{
	dataBridge->glibcNumericLocale.values[0].string = fDecimalPoint;
	dataBridge->glibcNumericLocale.values[1].string = fThousandsSep;
	dataBridge->glibcNumericLocale.values[2].string = fGrouping;
	fDataBridge = dataBridge;
}


status_t
ICUNumericData::SetTo(const Locale& locale, const char* posixLocaleName)
{
	status_t result = inherited::SetTo(locale, posixLocaleName);

	if (result == B_OK) {
		UErrorCode icuStatus = U_ZERO_ERROR;
		DecimalFormat* numberFormat = dynamic_cast<DecimalFormat*>(
			NumberFormat::createInstance(locale, UNUM_DECIMAL, icuStatus));
		if (!U_SUCCESS(icuStatus))
			return B_UNSUPPORTED;
		if (!numberFormat)
			return B_BAD_TYPE;
		const DecimalFormatSymbols* formatSymbols
			= numberFormat->getDecimalFormatSymbols();
		if (!formatSymbols)
			result = B_BAD_DATA;

		if (result == B_OK) {
			result = _SetLocaleconvEntry(formatSymbols, fDecimalPoint,
				DecimalFormatSymbols::kDecimalSeparatorSymbol);
			fDataBridge->glibcNumericLocale.values[3].word
				= (unsigned int)fDecimalPoint[0];
		}
		if (result == B_OK) {
			result = _SetLocaleconvEntry(formatSymbols, fThousandsSep,
				DecimalFormatSymbols::kGroupingSeparatorSymbol);
			fDataBridge->glibcNumericLocale.values[4].word
				= (unsigned int)fThousandsSep[0];
		}
		if (result == B_OK) {
			int32 groupingSize = numberFormat->getGroupingSize();
			if (groupingSize < 1)
				fGrouping[0] = '\0';
			else {
				fGrouping[0] = groupingSize;
				int32 secondaryGroupingSize
					= numberFormat->getSecondaryGroupingSize();
				if (secondaryGroupingSize < 1)
					fGrouping[1] = '\0';
				else {
					fGrouping[1] = secondaryGroupingSize;
					fGrouping[2] = '\0';
				}
			}
		}

		delete numberFormat;
	}

	return result;
}


status_t
ICUNumericData::SetToPosix()
{
	status_t result = inherited::SetToPosix();

	if (result == B_OK) {
		strcpy(fDecimalPoint, fDataBridge->posixLocaleConv->decimal_point);
		strcpy(fThousandsSep, fDataBridge->posixLocaleConv->thousands_sep);
		strcpy(fGrouping, fDataBridge->posixLocaleConv->grouping);
		fDataBridge->glibcNumericLocale.values[3].word
			= (unsigned int)fDecimalPoint[0];
		fDataBridge->glibcNumericLocale.values[4].word
			= (unsigned int)fThousandsSep[0];
	}

	return result;
}


const char*
ICUNumericData::GetLanginfo(int index)
{
	switch(index) {
		case RADIXCHAR:
			return fDecimalPoint;
		case THOUSEP:
			return fThousandsSep;
		default:
			return "";
	}
}


}	// namespace Libroot
}	// namespace BPrivate
