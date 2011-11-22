/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUCtypeData.h"

#include <langinfo.h>
#include <string.h>

#include <unicode/uchar.h>


namespace BPrivate {
namespace Libroot {


ICUCtypeData::ICUCtypeData(pthread_key_t tlsKey)
	:
	inherited(tlsKey),
	fDataBridge(NULL)
{
}


ICUCtypeData::~ICUCtypeData()
{
}


void
ICUCtypeData::Initialize(LocaleCtypeDataBridge* dataBridge)
{
	*dataBridge->addrOfClassInfoTable = &fClassInfo[128];
	*dataBridge->addrOfToLowerTable = &fToLowerMap[128];
	*dataBridge->addrOfToUpperTable = &fToUpperMap[128];
	fDataBridge = dataBridge;
}


status_t
ICUCtypeData::SetTo(const Locale& locale, const char* posixLocaleName)
{
	status_t result = inherited::SetTo(locale, posixLocaleName);
	if (result != B_OK)
		return result;

	UErrorCode icuStatus = U_ZERO_ERROR;

	ICUConverterRef converterRef;
	result = _GetConverter(converterRef);
	if (result != B_OK)
		return result;

	UConverter* converter = converterRef->Converter();

	ucnv_reset(converter);
	char buffer[] = { 0, 0 };
	for (int i = 0; i < 256; ++i) {
		const char* source = buffer;
		buffer[0] = (char)i;
		buffer[1] = '\0';
		icuStatus = U_ZERO_ERROR;
		UChar32 unicodeChar
			= ucnv_getNextUChar(converter, &source, source + 1, &icuStatus);

		unsigned short classInfo = 0;
		unsigned int toLower = i;
		unsigned int toUpper = i;
		if (U_SUCCESS(icuStatus)) {
			if (u_isblank(unicodeChar))
				classInfo |= _ISblank;
			if (u_charType(unicodeChar) == U_CONTROL_CHAR)
				classInfo |= _IScntrl;
			if (u_ispunct(unicodeChar))
				classInfo |= _ISpunct;
			if (u_hasBinaryProperty(unicodeChar, UCHAR_POSIX_ALNUM))
				classInfo |= _ISalnum;
			if (u_isUUppercase(unicodeChar))
				classInfo |= _ISupper;
			if (u_isULowercase(unicodeChar))
				classInfo |= _ISlower;
			if (u_isUAlphabetic(unicodeChar))
				classInfo |= _ISalpha;
			if (u_isdigit(unicodeChar))
				classInfo |= _ISdigit;
			if (u_isxdigit(unicodeChar))
				classInfo |= _ISxdigit;
			if (u_isUWhiteSpace(unicodeChar))
				classInfo |= _ISspace;
			if (u_hasBinaryProperty(unicodeChar, UCHAR_POSIX_PRINT))
				classInfo |= _ISprint;
			if (u_hasBinaryProperty(unicodeChar, UCHAR_POSIX_GRAPH))
				classInfo |= _ISgraph;

			UChar lowerChar = u_tolower(unicodeChar);
			icuStatus = U_ZERO_ERROR;
			ucnv_fromUChars(converter, buffer, 1, &lowerChar, 1, &icuStatus);
			if (U_SUCCESS(icuStatus))
				toLower = (unsigned char)buffer[0];

			UChar upperChar = u_toupper(unicodeChar);
			icuStatus = U_ZERO_ERROR;
			ucnv_fromUChars(converter, buffer, 1, &upperChar, 1, &icuStatus);
			if (U_SUCCESS(icuStatus))
				toUpper = (unsigned char)buffer[0];
		}
		fClassInfo[i + 128] = classInfo;
		fToLowerMap[i + 128] = toLower;
		fToUpperMap[i + 128] = toUpper;
		if (i >= 128 && i < 255) {
			// mirror upper half at negative indices (except for -1 [=EOF])
			fClassInfo[i - 128] = classInfo;
			fToLowerMap[i - 128] = toLower;
			fToUpperMap[i - 128] = toUpper;
		}
	}

	return B_OK;
}


status_t
ICUCtypeData::SetToPosix()
{
	status_t result = inherited::SetToPosix();

	if (result == B_OK) {
		memcpy(fClassInfo, fDataBridge->posixClassInfo, sizeof(fClassInfo));
		memcpy(fToLowerMap, fDataBridge->posixToLowerMap, sizeof(fToLowerMap));
		memcpy(fToUpperMap, fDataBridge->posixToUpperMap, sizeof(fToUpperMap));
	}

	return result;
}


int
ICUCtypeData::IsWCType(wint_t wc, wctype_t charClass)
{
	if (wc == WEOF)
		return 0;

	switch (charClass) {
		case _ISalnum:
			return u_hasBinaryProperty(wc, UCHAR_POSIX_ALNUM);
		case _ISalpha:
			return u_isUAlphabetic(wc);
		case _ISblank:
			return u_isblank(wc);
		case _IScntrl:
			return u_charType(wc) == U_CONTROL_CHAR;
		case _ISdigit:
			return u_isdigit(wc);
		case _ISgraph:
			return u_hasBinaryProperty(wc, UCHAR_POSIX_GRAPH);
		case _ISlower:
			return u_isULowercase(wc);
		case _ISprint:
			return u_hasBinaryProperty(wc, UCHAR_POSIX_PRINT);
		case _ISpunct:
			return u_ispunct(wc);
		case _ISspace:
			return u_isUWhiteSpace(wc);
		case _ISupper:
			return u_isUUppercase(wc);
		case _ISxdigit:
			return u_isxdigit(wc);
		default:
			return 0;
	}
}


status_t
ICUCtypeData::ToWCTrans(wint_t wc, wctrans_t transition, wint_t& result)
{
	switch (transition) {
		case _ISlower:
			result = u_tolower(wc);
			return B_OK;
		case _ISupper:
			result = u_toupper(wc);
			return B_OK;
		default:
			return B_BAD_VALUE;
	}
}


const char*
ICUCtypeData::GetLanginfo(int index)
{
	switch(index) {
		case CODESET:
			return fGivenCharset;
		default:
			return "";
	}
}


}	// namespace Libroot
}	// namespace BPrivate
