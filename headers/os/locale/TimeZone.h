/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_ZONE_H
#define _TIME_ZONE_H


#include <String.h>


class BTimeZone {
public:
								BTimeZone(const char* zoneCode = NULL);
								~BTimeZone();

			const BString&		Code() const;
			const BString&		Name() const;
			const BString&		DaylightSavingName() const;
			const BString&		ShortName() const;
			const BString&		ShortDaylightSavingName() const;
			int					OffsetFromGMT() const;
			bool				SupportsDaylightSaving() const;

			status_t			InitCheck() const;

			status_t			SetTo(const char* zoneCode);

	static  const char*			kNameOfGmtZone;

private:
			BString				fCode;
			BString				fName;
			BString				fDaylightSavingName;
			BString				fShortName;
			BString				fShortDaylightSavingName;
			int					fOffsetFromGMT;
			bool				fSupportsDaylightSaving;

			status_t			fInitStatus;
};


#endif	// _TIME_ZONE_H
