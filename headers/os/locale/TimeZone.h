/*
 * Copyright 2010-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_ZONE_H
#define _TIME_ZONE_H


#include <String.h>


namespace icu {
	class Locale;
	class TimeZone;
}
class BLanguage;


class BTimeZone {
public:
								BTimeZone(const char* zoneID = NULL,
									const BLanguage* language = NULL);
								BTimeZone(const BTimeZone& other);
								~BTimeZone();

			BTimeZone&			operator=(const BTimeZone& source);

			const BString&		ID() const;
			const BString&		Name() const;
			const BString&		DaylightSavingName() const;
			const BString&		ShortName() const;
			const BString&		ShortDaylightSavingName() const;
			int					OffsetFromGMT() const;
			bool				SupportsDaylightSaving() const;

			status_t			InitCheck() const;

			status_t			SetTo(const char* zoneID,
									const BLanguage* language = NULL);

			status_t			SetLanguage(const BLanguage* language);

	static  const char*			kNameOfGmtZone;

			class Private;
private:
	friend	class Private;

			icu::TimeZone*		fICUTimeZone;
			icu::Locale*		fICULocale;
			status_t			fInitStatus;

	mutable uint32				fInitializedFields;
	mutable BString				fZoneID;
	mutable BString				fName;
	mutable BString				fDaylightSavingName;
	mutable BString				fShortName;
	mutable BString				fShortDaylightSavingName;
	mutable int					fOffsetFromGMT;
	mutable bool				fSupportsDaylightSaving;
};


#endif	// _TIME_ZONE_H
