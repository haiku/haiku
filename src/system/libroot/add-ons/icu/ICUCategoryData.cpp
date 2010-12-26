/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUCategoryData.h"

#include <string.h>

#include <unicode/uchar.h>


namespace BPrivate {
namespace Libroot {


ICUCategoryData::ICUCategoryData()
	:
	fConverter(NULL)
{
	*fPosixLocaleName = '\0';
	*fGivenCharset = '\0';
}


ICUCategoryData::~ICUCategoryData()
{
	if (fConverter)
		ucnv_close(fConverter);
}


status_t
ICUCategoryData::_SetupConverter()
{
	if (fConverter != NULL) {
		ucnv_close(fConverter);
		fConverter = NULL;
	}

	UErrorCode icuStatus = U_ZERO_ERROR;
	fConverter = ucnv_open(fGivenCharset, &icuStatus);
	if (fConverter == NULL)
		return B_NAME_NOT_FOUND;

	icuStatus = U_ZERO_ERROR;
	ucnv_setToUCallBack(fConverter, UCNV_TO_U_CALLBACK_STOP, NULL, NULL,
		NULL, &icuStatus);
	icuStatus = U_ZERO_ERROR;
	ucnv_setFromUCallBack(fConverter, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL,
		NULL, &icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	return B_OK;
}


status_t
ICUCategoryData::SetTo(const Locale& locale, const char* posixLocaleName)
{
	if (!posixLocaleName)
		return B_BAD_VALUE;

	fLocale = locale;
	strlcpy(fPosixLocaleName, posixLocaleName, skMaxPosixLocaleNameLen);
	*fGivenCharset = '\0';

	// POSIX locales often contain an embedded charset, but ICU does not
	// handle these within locales (that part of the name is simply
	// ignored).
	// We need to fetch the charset specification and lookup an appropriate
	// ICU charset converter. This converter will later be used to get info
	// about ctype properties.
	const char* charsetStart = strchr(fPosixLocaleName, '.');
	if (charsetStart) {
		++charsetStart;
		int l = 0;
		while (charsetStart[l] != '\0' && charsetStart[l] != '@')
			++l;
		snprintf(fGivenCharset, UCNV_MAX_CONVERTER_NAME_LENGTH, "%.*s", l,
			charsetStart);
	}
	if (!strlen(fGivenCharset))
		strcpy(fGivenCharset, "utf-8");

	return _SetupConverter();
}


status_t
ICUCategoryData::SetToPosix()
{
	fLocale = Locale::createFromName("en_US_POSIX");
	strcpy(fPosixLocaleName, "POSIX");
	strcpy(fGivenCharset, "US-ASCII");

	return _SetupConverter();
}


status_t
ICUCategoryData::_ConvertUnicodeStringToLocaleconvEntry(
	const UnicodeString& string, char* destination, int destinationSize,
	const char* defaultValue)
{
	status_t result = B_OK;
	UErrorCode icuStatus = U_ZERO_ERROR;

	ucnv_fromUChars(fConverter, destination, destinationSize,
		string.getBuffer(), string.length(), &icuStatus);
	if (!U_SUCCESS(icuStatus)) {
		switch (icuStatus) {
			case U_BUFFER_OVERFLOW_ERROR:
				result = B_NAME_TOO_LONG;
				break;
			case U_INVALID_CHAR_FOUND:
			case U_TRUNCATED_CHAR_FOUND:
			case U_ILLEGAL_CHAR_FOUND:
				result = B_BAD_DATA;
				break;
			default:
				result = B_ERROR;
				break;
		}
		strlcpy(destination, defaultValue, destinationSize);
	}

	return result;
}


}	// namespace Libroot
}	// namespace BPrivate
