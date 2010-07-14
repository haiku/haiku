#ifndef _COUNTRY_H_
#define _COUNTRY_H_


#include <SupportDefs.h>
#include <LocaleStrings.h>
#include <String.h>


class BBitmap;

namespace icu_44 {
	class DateFormat;
	class Locale;
}


enum {
	B_METRIC = 0,
	B_US
};

typedef enum {
	B_INVALID = B_BAD_DATA,
	B_AM_PM = 0,
	B_HOUR,
	B_MINUTE,
	B_SECOND
} BDateField;


class BCountry {
	public:
		BCountry(const char* languageCode, const char* countryCode);
		BCountry(const char* languageAndCountryCode = "en_US");
		virtual 		~BCountry();

		virtual bool 	Name(BString&) const;
		const char*		Code() const;
		status_t		GetIcon(BBitmap* result);

		const char*		GetString(uint32 id) const;

		// date & time

		virtual void	FormatDate(char* string, size_t maxSize, time_t time,
			bool longFormat);
		virtual void	FormatDate(BString* string, time_t time,
			bool longFormat);
		virtual void	FormatTime(char* string, size_t maxSize, time_t time,
			bool longFormat);
		virtual void	FormatTime(BString* string, time_t time,
			bool longFormat);
		status_t		FormatTime(BString* string, int*& fieldPositions,
			int& fieldCount, time_t time, bool longFormat);
		status_t		TimeFields(BDateField*& fields, int& fieldCount,
			bool longFormat);

		bool		DateFormat(BString&, bool longFormat) const;
		void		SetDateFormat(const char* formatString,
						bool longFormat = true);
		void		SetTimeFormat(const char* formatString,
						bool longFormat = true);
		bool		TimeFormat(BString&, bool longFormat) const;
		const char*	DateSeparator() const;
		const char*	TimeSeparator() const;

		int			StartOfWeek();

		// numbers

		virtual void FormatNumber(char* string, size_t maxSize, double value);
		virtual status_t FormatNumber(BString* string, double value);
		virtual void FormatNumber(char* string, size_t maxSize, int32 value);
		virtual void FormatNumber(BString* string, int32 value);

		bool		DecimalPoint(BString&) const;
		bool		ThousandsSeparator(BString&) const;
		bool		Grouping(BString&) const;

		bool		PositiveSign(BString&) const;
		bool		NegativeSign(BString&) const;

		// measurements

		virtual int8	Measurement() const;

		// monetary

		virtual ssize_t	FormatMonetary(char* string, size_t maxSize,
			double value);
		virtual ssize_t	FormatMonetary(BString* string, double value);

		bool		CurrencySymbol(BString&) const;
		bool		InternationalCurrencySymbol(BString&) const;
		bool		MonDecimalPoint(BString&) const;
		bool		MonThousandsSeparator(BString&) const;
		bool		MonGrouping(BString&) const;
		virtual int32	MonFracDigits() const;

	private:
		icu_44::DateFormat* fICULongDateFormatter;
		icu_44::DateFormat* fICUShortDateFormatter;
		icu_44::DateFormat* fICULongTimeFormatter;
		icu_44::DateFormat* fICUShortTimeFormatter;
		const char**	fStrings;
		icu_44::Locale* fICULocale;
};

#endif	/* _COUNTRY_H_ */
