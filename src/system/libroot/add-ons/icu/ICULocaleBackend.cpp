/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICULocaleBackend.h"

#include <new>

#include <langinfo.h>
#include <locale.h>
#include <string.h>

#include <ErrnoMaintainer.h>


namespace BPrivate {
namespace Libroot {


extern "C" LocaleBackend*
CreateInstance()
{
	return new(std::nothrow) ICULocaleBackend();
}


ICULocaleBackend::ICULocaleBackend()
	:
	fThreadLocalStorageKey(_CreateThreadLocalStorageKey()),
	fCollateData(fThreadLocalStorageKey),
	fCtypeData(fThreadLocalStorageKey),
	fMessagesData(fThreadLocalStorageKey),
	fMonetaryData(fThreadLocalStorageKey, fLocaleConv),
	fNumericData(fThreadLocalStorageKey, fLocaleConv),
	fTimeData(fThreadLocalStorageKey, fLCTimeInfo),
	fTimeConversion(fTimeData)
{
}


ICULocaleBackend::~ICULocaleBackend()
{
	pthread_key_delete(fThreadLocalStorageKey);
}


void
ICULocaleBackend::Initialize(LocaleDataBridge* dataBridge)
{
	ErrnoMaintainer errnoMaintainer;

	fCtypeData.Initialize(&dataBridge->ctypeDataBridge);
	fMessagesData.Initialize(&dataBridge->messagesDataBridge);
	fMonetaryData.Initialize(&dataBridge->monetaryDataBridge);
	fNumericData.Initialize(&dataBridge->numericDataBridge);
	fTimeData.Initialize(&dataBridge->timeDataBridge);
	fTimeConversion.Initialize(&dataBridge->timeConversionDataBridge);

	fPosixLanginfo = dataBridge->posixLanginfo;

	_SetPosixLocale(LC_ALL);
}


const char*
ICULocaleBackend::SetLocale(int category, const char* posixLocaleName)
{
	ErrnoMaintainer errnoMaintainer;

	if (posixLocaleName == NULL)
		return _QueryLocale(category);

	if (strcasecmp(posixLocaleName, "POSIX") == 0
		|| strcasecmp(posixLocaleName, "C") == 0)
		return _SetPosixLocale(category);

	Locale locale = Locale::createCanonical(posixLocaleName);
	switch (category) {
		case LC_ALL:
			if (fCollateData.SetTo(locale, posixLocaleName) != B_OK
				|| fCtypeData.SetTo(locale, posixLocaleName) != B_OK
				|| fMessagesData.SetTo(locale, posixLocaleName) != B_OK
				|| fMonetaryData.SetTo(locale, posixLocaleName) != B_OK
				|| fNumericData.SetTo(locale, posixLocaleName) != B_OK
				|| fTimeData.SetTo(locale, posixLocaleName) != B_OK)
				return NULL;
			break;
		case LC_COLLATE:
			if (fCollateData.SetTo(locale, posixLocaleName) != B_OK)
				return NULL;
			break;
		case LC_CTYPE:
			if (fCtypeData.SetTo(locale, posixLocaleName) != B_OK)
				return NULL;
			break;
		case LC_MESSAGES:
			if (fMessagesData.SetTo(locale, posixLocaleName) != B_OK)
				return NULL;
			break;
		case LC_MONETARY:
			if (fMonetaryData.SetTo(locale, posixLocaleName) != B_OK)
				return NULL;
			break;
		case LC_NUMERIC:
			if (fNumericData.SetTo(locale, posixLocaleName) != B_OK)
				return NULL;
			break;
		case LC_TIME:
			if (fTimeData.SetTo(locale, posixLocaleName) != B_OK)
				return NULL;
			break;
		default:
			// unsupported category
			return NULL;
	}

	return posixLocaleName;
}


const struct lconv*
ICULocaleBackend::LocaleConv()
{
	return &fLocaleConv;
}


const struct lc_time_t*
ICULocaleBackend::LCTimeInfo()
{
	return &fLCTimeInfo;
}


int
ICULocaleBackend::IsWCType(wint_t wc, wctype_t charClass)
{
	ErrnoMaintainer errnoMaintainer;

	return fCtypeData.IsWCType(wc, charClass);
}


status_t
ICULocaleBackend::ToWCTrans(wint_t wc, wctrans_t transition, wint_t& result)
{
	ErrnoMaintainer errnoMaintainer;

	return fCtypeData.ToWCTrans(wc, transition, result);
}


status_t
ICULocaleBackend::MultibyteToWchar(wchar_t* wcOut, const char* mb,
	size_t mbLength, mbstate_t* mbState, size_t& lengthOut)
{
	ErrnoMaintainer errnoMaintainer;

	return fCtypeData.MultibyteToWchar(wcOut, mb, mbLength, mbState, lengthOut);
}


status_t
ICULocaleBackend::WcharToMultibyte(char* mbOut, wchar_t wc, mbstate_t* mbState,
	size_t& lengthOut)
{
	ErrnoMaintainer errnoMaintainer;

	return fCtypeData.WcharToMultibyte(mbOut, wc, mbState, lengthOut);
}


const char*
ICULocaleBackend::GetLanginfo(int index)
{
	ErrnoMaintainer errnoMaintainer;

	switch(index) {
		case CODESET:
			return fCtypeData.GetLanginfo(index);

		case D_T_FMT:
		case D_FMT:
		case T_FMT:
		case T_FMT_AMPM:
		case AM_STR:
		case PM_STR:
		case DAY_1:
		case DAY_2:
		case DAY_3:
		case DAY_4:
		case DAY_5:
		case DAY_6:
		case DAY_7:
		case ABDAY_1:
		case ABDAY_2:
		case ABDAY_3:
		case ABDAY_4:
		case ABDAY_5:
		case ABDAY_6:
		case ABDAY_7:
		case MON_1:
		case MON_2:
		case MON_3:
		case MON_4:
		case MON_5:
		case MON_6:
		case MON_7:
		case MON_8:
		case MON_9:
		case MON_10:
		case MON_11:
		case MON_12:
		case ABMON_1:
		case ABMON_2:
		case ABMON_3:
		case ABMON_4:
		case ABMON_5:
		case ABMON_6:
		case ABMON_7:
		case ABMON_8:
		case ABMON_9:
		case ABMON_10:
		case ABMON_11:
		case ABMON_12:
			return fTimeData.GetLanginfo(index);

		case ERA:
		case ERA_D_FMT:
		case ERA_D_T_FMT:
		case ERA_T_FMT:
		case ALT_DIGITS:
			return fPosixLanginfo[index];

		case RADIXCHAR:
		case THOUSEP:
			return fNumericData.GetLanginfo(index);

		case YESEXPR:
		case NOEXPR:
			return fMessagesData.GetLanginfo(index);

		case CRNCYSTR:
			return fMonetaryData.GetLanginfo(index);

		default:
			return "";
	}
}


status_t
ICULocaleBackend::Strcoll(const char* a, const char* b, int& result)
{
	ErrnoMaintainer errnoMaintainer;

	return fCollateData.Strcoll(a, b, result);
}


status_t
ICULocaleBackend::Strxfrm(char* out, const char* in, size_t size,
	size_t& outSize)
{
	ErrnoMaintainer errnoMaintainer;

	return fCollateData.Strxfrm(out, in, size, outSize);
}


status_t
ICULocaleBackend::TZSet(const char* timeZoneID, const char* tz)
{
	ErrnoMaintainer errnoMaintainer;

	return fTimeConversion.TZSet(timeZoneID, tz);
}


status_t
ICULocaleBackend::Localtime(const time_t* inTime, struct tm* tmOut)
{
	ErrnoMaintainer errnoMaintainer;

	return fTimeConversion.Localtime(inTime, tmOut);
}


status_t
ICULocaleBackend::Gmtime(const time_t* inTime, struct tm* tmOut)
{
	ErrnoMaintainer errnoMaintainer;

	return fTimeConversion.Gmtime(inTime, tmOut);
}


status_t
ICULocaleBackend::Mktime(struct tm* inOutTm, time_t& timeOut)
{
	ErrnoMaintainer errnoMaintainer;

	return fTimeConversion.Mktime(inOutTm, timeOut);
}


const char*
ICULocaleBackend::_QueryLocale(int category)
{
	switch (category) {
		case LC_ALL:
		{
			// if all categories have the same locale, return that
			const char* locale = fCollateData.PosixLocaleName();
			if (strcmp(locale, fCtypeData.PosixLocaleName()) == 0
				&& strcmp(locale, fMessagesData.PosixLocaleName()) == 0
				&& strcmp(locale, fMonetaryData.PosixLocaleName()) == 0
				&& strcmp(locale, fNumericData.PosixLocaleName()) == 0
				&& strcmp(locale, fTimeData.PosixLocaleName()) == 0)
				return locale;

			// build a string with all info (at least glibc does it, too)
			snprintf(fLocaleDescription, sizeof(fLocaleDescription),
				"LC_COLLATE=%s;LC_CTYPE=%s;LC_MESSAGES=%s;LC_MONETARY=%s;"
				"LC_NUMERIC=%s;LC_TIME=%s",
				fCollateData.PosixLocaleName(), fCtypeData.PosixLocaleName(),
				fMessagesData.PosixLocaleName(),
				fMonetaryData.PosixLocaleName(), fNumericData.PosixLocaleName(),
				fTimeData.PosixLocaleName());
			return fLocaleDescription;
		}
		case LC_COLLATE:
			return fCollateData.PosixLocaleName();
		case LC_CTYPE:
			return fCtypeData.PosixLocaleName();
		case LC_MESSAGES:
			return fMessagesData.PosixLocaleName();
		case LC_MONETARY:
			return fMonetaryData.PosixLocaleName();
		case LC_NUMERIC:
			return fNumericData.PosixLocaleName();
		case LC_TIME:
			return fTimeData.PosixLocaleName();
		default:
			// unsupported category
			return NULL;
	}
}


const char*
ICULocaleBackend::_SetPosixLocale(int category)
{
	switch (category) {
		case LC_ALL:
			if (fCollateData.SetToPosix() != B_OK
				|| fCtypeData.SetToPosix() != B_OK
				|| fMessagesData.SetToPosix() != B_OK
				|| fMonetaryData.SetToPosix() != B_OK
				|| fNumericData.SetToPosix() != B_OK
				|| fTimeData.SetToPosix() != B_OK)
				return NULL;
			break;
		case LC_COLLATE:
			if (fCollateData.SetToPosix() != B_OK)
				return NULL;
			break;
		case LC_CTYPE:
			if (fCtypeData.SetToPosix() != B_OK)
				return NULL;
			break;
		case LC_MESSAGES:
			if (fMessagesData.SetToPosix() != B_OK)
				return NULL;
			break;
		case LC_MONETARY:
			if (fMonetaryData.SetToPosix() != B_OK)
				return NULL;
			break;
		case LC_NUMERIC:
			if (fNumericData.SetToPosix() != B_OK)
				return NULL;
			break;
		case LC_TIME:
			if (fTimeData.SetToPosix() != B_OK)
				return NULL;
			break;
		default:
			// unsupported category
			return NULL;
	}

	return "POSIX";
}


pthread_key_t
ICULocaleBackend::_CreateThreadLocalStorageKey()
{
	pthread_key_t key;

	pthread_key_create(&key, ICULocaleBackend::_DestroyThreadLocalStorageValue);

	return key;
}


void ICULocaleBackend::_DestroyThreadLocalStorageValue(void* value)
{
	ICUThreadLocalStorageValue* tlsValue
		= static_cast<ICUThreadLocalStorageValue*>(value);

	delete tlsValue;
}


}	// namespace Libroot
}	// namespace BPrivate
