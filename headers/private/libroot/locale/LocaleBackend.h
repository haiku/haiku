/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCALE_BACKEND_H
#define _LOCALE_BACKEND_H


#include <SupportDefs.h>

#include <locale.h>
#include <time.h>
#include <wctype.h>


struct lc_time_t;


namespace BPrivate {
namespace Libroot {


struct LocaleCtypeDataBridge {
private:
	const unsigned short*		localClassInfoTable;
	const int*					localToLowerTable;
	const int*					localToUpperTable;
	unsigned short				localMbCurMax;

public:
	const unsigned short**		addrOfClassInfoTable;
	const int**					addrOfToLowerTable;
	const int**					addrOfToUpperTable;
	unsigned short*				addrOfMbCurMax;

	const unsigned short* const	posixClassInfo;
	const int* const			posixToLowerMap;
	const int* const			posixToUpperMap;

	bool						isGlobal;

	LocaleCtypeDataBridge(bool isGlobal);

	void ApplyToCurrentThread();
};


struct LocaleMessagesDataBridge {
	const char** const 	posixLanginfo;

	LocaleMessagesDataBridge();
};


struct LocaleMonetaryDataBridge {
	const struct lconv* const posixLocaleConv;

	LocaleMonetaryDataBridge();
};


struct LocaleNumericDataBridge {
public:
	const struct lconv* const 	posixLocaleConv;

	LocaleNumericDataBridge(bool isGlobal);
	~LocaleNumericDataBridge();
};


struct LocaleTimeDataBridge {
	const struct lc_time_t* const posixLCTimeInfo;

	LocaleTimeDataBridge();
};


struct TimeConversionDataBridge {
	static const int32 		kTZNameLength = 64;

private:
	int						localDaylight;
	long					localTimezone;
	char*					localTZName[2];
	char					localTZName0[kTZNameLength];
	char					localTZName1[kTZNameLength];

public:
	int*					addrOfDaylight;
	long*					addrOfTimezone;
	char**					addrOfTZName;

	TimeConversionDataBridge(bool isGlobal);
};


struct LocaleDataBridge {
	LocaleCtypeDataBridge		ctypeDataBridge;
	LocaleMessagesDataBridge	messagesDataBridge;
	LocaleMonetaryDataBridge	monetaryDataBridge;
	LocaleNumericDataBridge		numericDataBridge;
	LocaleTimeDataBridge		timeDataBridge;
	TimeConversionDataBridge	timeConversionDataBridge;
	const char** const			posixLanginfo;

	LocaleDataBridge(bool isGlobal);

	void ApplyToCurrentThread();
};


class LocaleBackend {
public:
								LocaleBackend();
	virtual						~LocaleBackend();

	virtual	const char*			SetLocale(int category, const char* locale) = 0;
	virtual	const struct lconv*	LocaleConv() = 0;
	virtual	const struct lc_time_t*	LCTimeInfo() = 0;

	virtual	int					IsWCType(wint_t wc, wctype_t charClass) = 0;
	virtual	status_t			ToWCTrans(wint_t wc, wctrans_t transition,
									wint_t& result) = 0;

	virtual status_t			MultibyteToWchar(wchar_t* wcOut, const char* mb,
									size_t mbLength, mbstate_t* mbState,
									size_t& lengthOut) = 0;
	virtual status_t			MultibyteStringToWchar(wchar_t* wcDest,
									size_t wcDestLength, const char** mbSource,
									size_t mbSourceLength, mbstate_t* mbState,
									size_t& lengthOut) = 0;
	virtual status_t			WcharToMultibyte(char* mbOut, wchar_t wc,
									mbstate_t* mbState, size_t& lengthOut) = 0;
	virtual status_t			WcharStringToMultibyte(char* mbDest,
									size_t mbDestLength,
									const wchar_t** wcSource,
									size_t wcSourceLength, mbstate_t* mbState,
									size_t& lengthOut) = 0;

	virtual	const char*			GetLanginfo(int index) = 0;

	virtual	status_t			Strcoll(const char* a, const char* b,
									int& out) = 0;
	virtual status_t			Strxfrm(char* out, const char* in,
									size_t outSize, size_t& requiredSize) = 0;
	virtual	status_t			Wcscoll(const wchar_t* a, const wchar_t* b,
									int& out) = 0;
	virtual status_t			Wcsxfrm(wchar_t* out, const wchar_t* in,
									size_t outSize, size_t& requiredSize) = 0;

	virtual status_t			TZSet(const char* timeZoneID,
									const char* tz) = 0;
	virtual	status_t			Localtime(const time_t* inTime,
									struct tm* tmOut) = 0;
	virtual	status_t			Gmtime(const time_t* inTime,
									struct tm* tmOut) = 0;
	virtual status_t			Mktime(struct tm* inOutTm, time_t& timeOut) = 0;

	virtual status_t			Timegm(struct tm* inOutTm, time_t& timeOut) = 0;

	virtual void				Initialize(LocaleDataBridge* dataBridge) = 0;

	static	status_t			LoadBackend();
	static  status_t			CreateBackend(LocaleBackend*& backendOut);
	static  void				DestroyBackend(LocaleBackend* instance);
};


// The real struct behind locale_t
struct LocaleBackendData {
	int magic;
	LocaleBackend* backend;
	LocaleDataBridge* databridge;
};


LocaleBackendData* GetCurrentLocaleInfo();
void SetCurrentLocaleInfo(LocaleBackendData* newLocale);
LocaleBackend* GetCurrentLocaleBackend();
extern LocaleBackend* gGlobalLocaleBackend;
extern LocaleDataBridge gGlobalLocaleDataBridge;


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _LOCALE_BACKEND_H
