/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUCtypeData.h"

#include <langinfo.h>
#include <string.h>

#include <unicode/uchar.h>


//#define TRACE_CTYPE
#ifdef TRACE_CTYPE
#	include <OS.h>
#	define TRACE(x) debug_printf x
#else
#	define TRACE(x) ;
#endif


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

	fDataBridge->setMbCurMax(ucnv_getMaxCharSize(converter));

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

		fDataBridge->setMbCurMax(1);
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


status_t
ICUCtypeData::MultibyteToWchar(wchar_t* wcOut, const char* mb, size_t mbLen,
	mbstate_t* mbState, size_t& lengthOut)
{
	ICUConverterRef converterRef;
	status_t result = _GetConverterForMbState(mbState, converterRef);
	if (result != B_OK) {
		TRACE(("MultibyteToWchar(): couldn't get converter for ID %d - %lx\n",
			mbState->converterID, result));
		return result;
	}

	UConverter* converter = converterRef->Converter();

	// do the conversion
	UErrorCode icuStatus = U_ZERO_ERROR;

	const char* buffer = mb;
	UChar targetBuffer[2];
	UChar* target = targetBuffer;
	ucnv_toUnicode(converter, &target, target + 1, &buffer, buffer + mbLen,
		NULL, FALSE, &icuStatus);
	size_t sourceLengthUsed = buffer - mb;
	size_t targetLengthUsed = (size_t)(target - targetBuffer);

	if (icuStatus == U_BUFFER_OVERFLOW_ERROR && targetLengthUsed > 0) {
		// we've got one character, which is all that we wanted
		icuStatus = U_ZERO_ERROR;
	}

	UChar32 unicodeChar = 0xBADBEEF;

	if (!U_SUCCESS(icuStatus)) {
		// conversion failed because of illegal character sequence
		TRACE(("MultibyteToWchar(): illegal character sequence\n"));
		ucnv_resetToUnicode(converter);
		result = B_BAD_DATA;
	} else 	if (targetLengthUsed == 0) {
		TRACE(("MultibyteToWchar(): incomplete character (len=%lu)\n", mbLen));
		for (size_t i = 0; i < mbLen; ++i)
			TRACE(("\tbyte %lu: %x\n", i, mb[i]));
		mbState->count = sourceLengthUsed;
		result = B_BAD_INDEX;
	} else {
		U16_GET(targetBuffer, 0, 0, 2, unicodeChar);

		if (unicodeChar == 0) {
			// reset to initial state
			_DropConverterFromMbState(mbState);
			memset(mbState, 0, sizeof(mbstate_t));
			lengthOut = 0;
		} else {
			mbState->count = 0;
			lengthOut = sourceLengthUsed;
		}

		if (wcOut != NULL)
			*wcOut = unicodeChar;

		result = B_OK;
	}

	return result;
}


status_t
ICUCtypeData::WcharToMultibyte(char* mbOut, wchar_t wc, mbstate_t* mbState,
	size_t& lengthOut)
{
	ICUConverterRef converterRef;
	status_t result = _GetConverterForMbState(mbState, converterRef);
	if (result != B_OK) {
		TRACE(("WcharToMultibyte(): couldn't get converter for ID %d - %lx\n",
			mbState->converterID, result));
		return result;
	}

	UConverter* converter = converterRef->Converter();

	// do the conversion
	UErrorCode icuStatus = U_ZERO_ERROR;
	lengthOut = ucnv_fromUChars(converter, mbOut, MB_LEN_MAX, (UChar*)&wc,
		1, &icuStatus);

	if (!U_SUCCESS(icuStatus)) {
		if (icuStatus == U_ILLEGAL_ARGUMENT_ERROR) {
			// bad converter (shouldn't really happen)
			TRACE(("MultibyteToWchar(): bad converter\n"));
			return B_BAD_VALUE;
		}

		// conversion failed because of illegal/unmappable character
		TRACE(("MultibyteToWchar(): illegal character sequence\n"));
		ucnv_resetFromUnicode(converter);
		return B_BAD_DATA;
	}

	if (wc == 0) {
		// reset to initial state
		_DropConverterFromMbState(mbState);
		memset(mbState, 0, sizeof(mbstate_t));
	}

	return B_OK;
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


status_t
ICUCtypeData::_GetConverterForMbState(mbstate_t* mbState,
	ICUConverterRef& converterRefOut)
{
	ICUConverterRef converterRef;
	status_t result = ICUConverterManager::Instance()->GetConverter(
		mbState->converterID, converterRef);
	if (result == B_OK) {
		if (strcmp(converterRef->Charset(), fGivenCharset) == 0) {
			converterRefOut = converterRef;
			return B_OK;
		}

		// charset no longer matches the converter, we need to dump it and
		// create a new one
		_DropConverterFromMbState(mbState);
	}

	// create a new converter for the current charset
	result = ICUConverterManager::Instance()->CreateConverter(fGivenCharset,
		converterRef, mbState->converterID);
	if (result != B_OK)
		return result;

	converterRefOut = converterRef;

	return B_OK;
}


status_t
ICUCtypeData::_DropConverterFromMbState(mbstate_t* mbState)
{
	ICUConverterManager::Instance()->DropConverter(mbState->converterID);
	mbState->converterID = 0;

	return B_OK;
}


}	// namespace Libroot
}	// namespace BPrivate
