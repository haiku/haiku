/*
 * Copyright 2003-2011, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _FORMATTING_CONVENTIONS_H_
#define _FORMATTING_CONVENTIONS_H_


#include <Archivable.h>
#include <List.h>
#include <LocaleStrings.h>
#include <String.h>
#include <SupportDefs.h>


class BBitmap;
class BLanguage;
class BMessage;

namespace icu {
	class DateFormat;
	class Locale;
}


enum BMeasurementKind {
	B_METRIC = 0,
	B_US
};


enum BDateFormatStyle {
	B_FULL_DATE_FORMAT = 0,
	B_LONG_DATE_FORMAT,
	B_MEDIUM_DATE_FORMAT,
	B_SHORT_DATE_FORMAT,

	B_DATE_FORMAT_STYLE_COUNT
};


enum BTimeFormatStyle {
	B_FULL_TIME_FORMAT = 0,
	B_LONG_TIME_FORMAT,
	B_MEDIUM_TIME_FORMAT,
	B_SHORT_TIME_FORMAT,

	B_TIME_FORMAT_STYLE_COUNT
};


class BFormattingConventions : public BArchivable {
public:
								BFormattingConventions(const char* id = NULL);
								BFormattingConventions(
									const BFormattingConventions& other);
								BFormattingConventions(const BMessage* archive);

								BFormattingConventions& operator=(
									const BFormattingConventions& other);

								~BFormattingConventions();

								bool operator==(
									const BFormattingConventions& other) const;
								bool operator!=(
									const BFormattingConventions& other) const;

			const char*			ID() const;
			const char*			LanguageCode() const;
			const char*			CountryCode() const;

			status_t			GetNativeName(BString& name) const;
			status_t			GetName(BString& name,
									const BLanguage* displayLanguage = NULL
									) const;

			const char*			GetString(uint32 id) const;

			status_t			GetDateFormat(BDateFormatStyle style,
									BString& outFormat) const;
			status_t			GetTimeFormat(BTimeFormatStyle style,
									BString& outFormat) const;
			status_t			GetNumericFormat(BString& outFormat) const;
			status_t			GetMonetaryFormat(BString& outFormat) const;

			void				SetExplicitDateFormat(BDateFormatStyle style,
									const BString& format);
			void				SetExplicitTimeFormat(BTimeFormatStyle style,
									const BString& format);
			void				SetExplicitNumericFormat(const BString& format);
			void				SetExplicitMonetaryFormat(
									const BString& format);

			BMeasurementKind	MeasurementKind() const;

			bool				UseStringsFromPreferredLanguage() const;
			void				SetUseStringsFromPreferredLanguage(bool value);

			bool				Use24HourClock() const;
			void				SetExplicitUse24HourClock(bool value);
			void				UnsetExplicitUse24HourClock();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

			class Private;
private:
	friend	class Private;

	mutable	BString				fCachedDateFormats[B_DATE_FORMAT_STYLE_COUNT];
	mutable	BString				fCachedTimeFormats[B_TIME_FORMAT_STYLE_COUNT];
	mutable	BString				fCachedNumericFormat;
	mutable	BString				fCachedMonetaryFormat;
	mutable	int8				fCachedUse24HourClock;

			BString				fExplicitDateFormats[B_DATE_FORMAT_STYLE_COUNT];
			BString				fExplicitTimeFormats[B_TIME_FORMAT_STYLE_COUNT];
			BString				fExplicitNumericFormat;
			BString				fExplicitMonetaryFormat;
			int8				fExplicitUse24HourClock;

			bool				fUseStringsFromPreferredLanguage;

			icu::Locale*		fICULocale;
};


#endif	/* _FORMATTING_CONVENTIONS_H_ */
