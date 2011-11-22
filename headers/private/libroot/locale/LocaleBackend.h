/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCALE_BACKEND_H
#define _LOCALE_BACKEND_H


#include <SupportDefs.h>

#include <time.h>
#include <wctype.h>


struct lconv;
struct lc_time_t;
struct locale_data;		// glibc


namespace BPrivate {
namespace Libroot {


struct LocaleCtypeDataBridge {
	const unsigned short**	addrOfClassInfoTable;
	const int**				addrOfToLowerTable;
	const int**				addrOfToUpperTable;

	const unsigned short*	posixClassInfo;
	const int*				posixToLowerMap;
	const int*				posixToUpperMap;

	LocaleCtypeDataBridge();

	void setMbCurMax(unsigned short mbCurMax);
};


struct LocaleMessagesDataBridge {
	const char** posixLanginfo;

	LocaleMessagesDataBridge();
};


struct LocaleMonetaryDataBridge {
	const struct lconv* posixLocaleConv;

	LocaleMonetaryDataBridge();
};


struct LocaleNumericDataBridge {
private:
	// struct used by glibc to store numeric locale data
	struct GlibcNumericLocale {
		const char* name;
		const char* filedata;
		off_t filesize;
		int mmaped;
		unsigned int usage_count;
		int use_translit;
		const char *options;
		unsigned int nstrings;
		union locale_data_value
		{
			const uint32_t* wstr;
			const char* string;
			unsigned int word;
		}
		values[6];
	};
	locale_data* originalGlibcLocale;

public:
	const struct lconv* posixLocaleConv;
	GlibcNumericLocale  glibcNumericLocale;

	LocaleNumericDataBridge();
	~LocaleNumericDataBridge();
};


struct LocaleTimeDataBridge {
	const struct lc_time_t* posixLCTimeInfo;

	LocaleTimeDataBridge();
};


struct TimeConversionDataBridge {
	int*					addrOfDaylight;
	long*					addrOfTimezone;
	char**					addrOfTZName;

	TimeConversionDataBridge();
};


struct LocaleDataBridge {
	LocaleCtypeDataBridge		ctypeDataBridge;
	LocaleMessagesDataBridge	messagesDataBridge;
	LocaleMonetaryDataBridge	monetaryDataBridge;
	LocaleNumericDataBridge		numericDataBridge;
	LocaleTimeDataBridge		timeDataBridge;
	TimeConversionDataBridge	timeConversionDataBridge;
	const char**				posixLanginfo;

	LocaleDataBridge();
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
	virtual status_t			WcharToMultibyte(char* mbOut, wchar_t wc,
									mbstate_t* mbState, size_t& lengthOut) = 0;

	virtual	const char*			GetLanginfo(int index) = 0;

	virtual	status_t			Strcoll(const char* a, const char* b,
									int& out) = 0;
	virtual status_t			Strxfrm(char* out, const char* in, size_t size,
									size_t& outSize) = 0;

	virtual status_t			TZSet(const char* timeZoneID,
									const char* tz) = 0;
	virtual	status_t			Localtime(const time_t* inTime,
									struct tm* tmOut) = 0;
	virtual	status_t			Gmtime(const time_t* inTime,
									struct tm* tmOut) = 0;
	virtual status_t			Mktime(struct tm* inOutTm, time_t& timeOut) = 0;

	virtual void				Initialize(LocaleDataBridge* dataBridge) = 0;

	static	status_t			LoadBackend();
};


extern LocaleBackend* gLocaleBackend;


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _LOCALE_BACKEND_H
