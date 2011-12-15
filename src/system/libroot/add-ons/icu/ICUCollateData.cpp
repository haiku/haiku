/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUCollateData.h"

#include <string.h>

#include <unicode/unistr.h>


namespace BPrivate {
namespace Libroot {


ICUCollateData::ICUCollateData(pthread_key_t tlsKey)
	:
	inherited(tlsKey),
	fCollator(NULL)
{
}


ICUCollateData::~ICUCollateData()
{
	delete fCollator;
}


status_t
ICUCollateData::SetTo(const Locale& locale, const char* posixLocaleName)
{
	status_t result = inherited::SetTo(locale, posixLocaleName);

	if (result == B_OK) {
		UErrorCode icuStatus = U_ZERO_ERROR;
		delete fCollator;
		fCollator = Collator::createInstance(fLocale, icuStatus);
		if (!U_SUCCESS(icuStatus))
			return B_NO_MEMORY;
	}

	return result;
}


status_t
ICUCollateData::SetToPosix()
{
	status_t result = inherited::SetToPosix();

	if (result == B_OK) {
		delete fCollator;
		fCollator = NULL;
	}

	return result;
}


status_t
ICUCollateData::Strcoll(const char* a, const char* b, int& result)
{
	if (strcmp(fPosixLocaleName, "POSIX") == 0) {
		// handle POSIX here as the collator ICU uses for that (english) is
		// incompatible in too many ways
		result = strcmp(a, b);
		for (const char* aIter = a; *aIter != 0; ++aIter) {
			if (*aIter < 0)
				return B_BAD_VALUE;
		}
		for (const char* bIter = b; *bIter != 0; ++bIter) {
			if (*bIter < 0)
				return B_BAD_VALUE;
		}
		return B_OK;
	}

	status_t status = B_OK;
	UErrorCode icuStatus = U_ZERO_ERROR;

	if (strcasecmp(fGivenCharset, "utf-8") == 0) {
		UCharIterator aIter, bIter;
		uiter_setUTF8(&aIter, a, -1);
		uiter_setUTF8(&bIter, b, -1);

		result = fCollator->compare(aIter, bIter, icuStatus);
	} else {
		UnicodeString unicodeA;
		UnicodeString unicodeB;

		if (_ToUnicodeString(a, unicodeA) != B_OK
			|| _ToUnicodeString(b, unicodeB) != B_OK) {
			status = B_BAD_VALUE;
		}

		result = fCollator->compare(unicodeA, unicodeB, icuStatus);
	}

	if (!U_SUCCESS(icuStatus))
		status = B_BAD_VALUE;

	return status;
}


status_t
ICUCollateData::Strxfrm(char* out, const char* in, size_t size, size_t& outSize)
{
	if (strcmp(fPosixLocaleName, "POSIX") == 0) {
		// handle POSIX here as the collator ICU uses for that (english) is
		// incompatible in too many ways
		outSize = strlcpy(out, in, size);
		for (const char* inIter = in; *inIter != 0; ++inIter) {
			if (*inIter < 0)
				return B_BAD_VALUE;
		}
		return B_OK;
	}

	if (in == NULL) {
		outSize = 0;
		return B_OK;
	}

	UErrorCode icuStatus = U_ZERO_ERROR;

	UnicodeString unicodeIn;
	if (_ToUnicodeString(in, unicodeIn) != B_OK)
		return B_BAD_VALUE;

	outSize = fCollator->getSortKey(unicodeIn, (uint8_t*)out, size);
	if (!U_SUCCESS(icuStatus))
		return B_BAD_VALUE;

	return B_OK;
}


status_t
ICUCollateData::_ToUnicodeString(const char* in, UnicodeString& out)
{
	out.remove();

	if (in == NULL)
		return B_OK;

	size_t inLen = strlen(in);
	if (inLen == 0)
		return B_OK;

	UConverter* converter;
	status_t result = _GetConverter(converter);
	if (result != B_OK)
		return result;

	UErrorCode icuStatus = U_ZERO_ERROR;
	int32_t outLen = ucnv_toUChars(converter, NULL, 0, in, inLen, &icuStatus);
	if (icuStatus != U_BUFFER_OVERFLOW_ERROR)
		return B_BAD_VALUE;
	if (outLen < 0)
		return B_ERROR;
	if (outLen == 0)
		return B_OK;

	UChar* outBuf = out.getBuffer(outLen + 1);
	icuStatus = U_ZERO_ERROR;
	outLen
		= ucnv_toUChars(converter, outBuf, outLen + 1, in, inLen, &icuStatus);
	if (!U_SUCCESS(icuStatus)) {
		out.releaseBuffer(0);
		return B_BAD_VALUE;
	}

	out.releaseBuffer(outLen);

	return B_OK;
}


}	// namespace Libroot
}	// namespace BPrivate
