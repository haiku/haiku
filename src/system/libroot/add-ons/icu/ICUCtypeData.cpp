/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUCtypeData.h"

#include <langinfo.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <unicode/uchar.h>

#include <Debug.h>


//#define TRACE_CTYPE
#undef TRACE
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

	UConverter* converter;
	result = _GetConverter(converter);
	if (result != B_OK)
		return result;

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
	UConverter* converter = NULL;
	status_t result = _GetConverterForMbState(mbState, converter);
	if (result != B_OK) {
		TRACE(("MultibyteToWchar(): couldn't get converter for mbstate %p - "
				"%lx\n", mbState, result));
		return result;
	}

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
		UChar32 unicodeChar = 0xBADBEEF;
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
ICUCtypeData::MultibyteStringToWchar(wchar_t* wcDest, size_t wcDestLength,
	const char** mbSource, size_t mbSourceLength, mbstate_t* mbState,
	size_t& lengthOut)
{
	UConverter* converter = NULL;
	status_t result = _GetConverterForMbState(mbState, converter);
	if (result != B_OK) {
		TRACE(("MultibyteStringToWchar(): couldn't get converter for mbstate %p"
				" - %lx\n", mbState, result));
		return result;
	}

	bool wcsIsTerminated = false;
	const char* source = *mbSource;
	const char* sourceEnd = source + mbSourceLength;
	if (sourceEnd < source) {
		// overflow, clamp to highest possible address
		sourceEnd = (const char*)-1;
	}

	if (wcDest == NULL) {
		// if there's no destination buffer, there's no length limit either
		wcDestLength = (size_t)-1;
	}

	UErrorCode icuStatus = U_ZERO_ERROR;
	size_t sourceLengthUsed = 0;
	for (lengthOut = 0; lengthOut < wcDestLength; ++lengthOut) {
		if (sourceLengthUsed >= mbSourceLength)
			break;
		UChar32 unicodeChar = ucnv_getNextUChar(converter, &source,
			std::min(source + MB_CUR_MAX, sourceEnd), &icuStatus);
		TRACE(("MultibyteStringToWchar() l:%lu wl:%lu s:%p se:%p sl:%lu slu:%lu"
				" uchar:%x st:%x\n", lengthOut, wcDestLength, source, sourceEnd,
			mbSourceLength, sourceLengthUsed, unicodeChar, icuStatus));
		if (!U_SUCCESS(icuStatus))
			break;
		sourceLengthUsed = source - *mbSource;
		if (wcDest != NULL)
			*wcDest++ = unicodeChar;
		if (unicodeChar == L'\0') {
			wcsIsTerminated = true;
			break;
		}
		icuStatus = U_ZERO_ERROR;
	}

	if (!U_SUCCESS(icuStatus)) {
		// conversion failed because of illegal character sequence
		TRACE(("MultibyteStringToWchar(): illegal character sequence\n"));
		ucnv_resetToUnicode(converter);
		result = B_BAD_DATA;
		if (wcDest != NULL)
			*mbSource = *mbSource + sourceLengthUsed;
	} else if (wcsIsTerminated) {
		// reset to initial state
		_DropConverterFromMbState(mbState);
		memset(mbState, 0, sizeof(mbstate_t));
		if (wcDest != NULL)
			*mbSource = NULL;
	} else {
		mbState->count = 0;
		if (wcDest != NULL)
			*mbSource = source;
	}

	return result;
}


status_t
ICUCtypeData::WcharToMultibyte(char* mbOut, wchar_t wc, mbstate_t* mbState,
	size_t& lengthOut)
{
	UConverter* converter = NULL;
	status_t result = _GetConverterForMbState(mbState, converter);
	if (result != B_OK) {
		TRACE(("WcharToMultibyte(): couldn't get converter for mbstate %p - "
				"%lx\n", mbState, result));
		return result;
	}

	// convert input from UTF-32 to UTF-16
	UChar ucharBuffer[2];
	size_t ucharLength;
	if (U_IS_BMP(wc)) {
		ucharBuffer[0] = wc;
		ucharLength = 1;
	} else {
		ucharBuffer[0] = U16_LEAD(wc);
		ucharBuffer[1] = U16_TRAIL(wc);
		ucharLength = 2;
	}

	// do the actual conversion
	UErrorCode icuStatus = U_ZERO_ERROR;
	size_t mbLength = mbOut == NULL ? 0 : MB_CUR_MAX;
	lengthOut = ucnv_fromUChars(converter, mbOut, mbLength, ucharBuffer,
		ucharLength, &icuStatus);
	TRACE(("WcharToMultibyte() l:%lu mb:%p ml:%lu uchar:%x st:%x\n", lengthOut,
		mbOut, mbLength, wc, icuStatus));

	if (icuStatus == U_BUFFER_OVERFLOW_ERROR && mbOut == NULL) {
		// we have no output buffer, so we ignore buffer overflows
		icuStatus = U_ZERO_ERROR;
	}

	if (!U_SUCCESS(icuStatus)) {
		if (icuStatus == U_ILLEGAL_ARGUMENT_ERROR) {
			// bad converter (shouldn't really happen)
			TRACE(("WcharToMultibyte(): bad converter\n"));
			return B_BAD_VALUE;
		}

		// conversion failed because of illegal/unmappable character
		TRACE(("WcharToMultibyte(): illegal character sequence\n"));
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


status_t
ICUCtypeData::WcharStringToMultibyte(char* mbDest, size_t mbDestLength,
	const wchar_t** wcSource, size_t wcSourceLength, mbstate_t* mbState,
	size_t& lengthOut)
{
	UConverter* converter = NULL;
	status_t result = _GetConverterForMbState(mbState, converter);
	if (result != B_OK) {
		TRACE(("WcharStringToMultibyte(): couldn't get converter for mbstate %p"
			" - %lx\n", mbState, result));
		return result;
	}

	bool mbsIsTerminated = false;
	const UChar32* source = (UChar32*)*wcSource;

	UErrorCode icuStatus = U_ZERO_ERROR;
	lengthOut = 0;
	size_t sourceLengthUsed = 0;
	for (; sourceLengthUsed < wcSourceLength; ++sourceLengthUsed, ++source) {
		if (mbDest != NULL && lengthOut >= mbDestLength)
			break;

		// convert input from UTF-32 to UTF-16
		UChar ucharBuffer[2];
		size_t ucharLength;
		if (U_IS_BMP(*source)) {
			ucharBuffer[0] = *source;
			ucharLength = 1;
		} else {
			ucharBuffer[0] = U16_LEAD(*source);
			ucharBuffer[1] = U16_TRAIL(*source);
			ucharLength = 2;
		}

		// do the actual conversion
		size_t destLength = mbDest == NULL ? 0 : mbDestLength - lengthOut;
		char buffer[MB_CUR_MAX];
		size_t mbLength = ucnv_fromUChars(converter,
			mbDest == NULL ? NULL : buffer, destLength, ucharBuffer,
			ucharLength, &icuStatus);
		TRACE(("WcharStringToMultibyte() l:%lu mb:%p ml:%lu s:%p ul:%lu slu:%lu"
				" uchar:%x st:%x\n", mbLength, mbDest, destLength, source,
			ucharLength, sourceLengthUsed, *source, icuStatus));

		if (icuStatus == U_BUFFER_OVERFLOW_ERROR) {
			// ignore buffer overflows ...
 			icuStatus = U_ZERO_ERROR;
 			// ... but stop if the output buffer has been exceeded
 			if (destLength > 0)
 				break;
		} else if (mbDest != NULL)
			memcpy(mbDest, buffer, mbLength);

		if (!U_SUCCESS(icuStatus))
			break;
		if (mbDest != NULL)
			mbDest += mbLength;
		if (*source == L'\0') {
			mbsIsTerminated = true;
			break;
		}
		lengthOut += mbLength;
		icuStatus = U_ZERO_ERROR;
	}

	if (!U_SUCCESS(icuStatus)) {
		// conversion failed because of illegal character sequence
		TRACE(("WcharStringToMultibyte(): illegal character sequence\n"));
		ucnv_resetFromUnicode(converter);
		result = B_BAD_DATA;
		if (mbDest != NULL)
			*wcSource = *wcSource + sourceLengthUsed;
	} else if (mbsIsTerminated) {
		// reset to initial state
		_DropConverterFromMbState(mbState);
		memset(mbState, 0, sizeof(mbstate_t));
		if (mbDest != NULL)
			*wcSource = NULL;
	} else {
		mbState->count = 0;
		if (mbDest != NULL)
			*wcSource = (wchar_t*)source;
	}

	return result;
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
	UConverter*& converterOut)
{
	if (strcmp(mbState->charset, fGivenCharset) == 0
			&& (char*)mbState->converter >= mbState->data
			&& (char*)mbState->converter < mbState->data + 8) {
		// charset matches and converter actually lives in *this* mbState,
		// so we can use it (if the converter points to the outside, it means
		// that the mbstate_t has been copied)
		converterOut = (UConverter*)mbState->converter;
		return B_OK;
	}

	// charset no longer matches the converter, we need to dump it and
	// create a new one
	_DropConverterFromMbState(mbState);

	// create a new converter for the current charset ...
	UConverter* icuConverter;
	status_t result = _GetConverter(icuConverter);
	if (result != B_OK)
		return result;

	// ... and clone it into the mbstate
	UErrorCode icuStatus = U_ZERO_ERROR;
	int32_t bufferSize = sizeof(mbState->data);
	UConverter* clone
		= ucnv_safeClone(icuConverter, mbState->data, &bufferSize, &icuStatus);
	if (clone == NULL || !U_SUCCESS(icuStatus))
		return B_ERROR;

	if ((char*)clone < mbState->data || (char*)clone >= mbState->data + 8) {
		// buffer is too small (shouldn't happen according to ICU docs)
		return B_NO_MEMORY;
	}

	strlcpy(mbState->charset, fGivenCharset, sizeof(mbState->charset));
	mbState->converter = clone;

	converterOut = clone;

	return B_OK;
}


status_t
ICUCtypeData::_DropConverterFromMbState(mbstate_t* mbState)
{
	if (mbState->converter != NULL)
		ucnv_close((UConverter*)mbState->converter);
	memset(mbState, 0, sizeof(mbstate_t));

	return B_OK;
}


}	// namespace Libroot
}	// namespace BPrivate
