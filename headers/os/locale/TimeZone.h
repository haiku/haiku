/*
Copyright 2010, Haiku, Inc.
Distributed under the terms of the MIT License.
*/


#ifndef __TIMEZONE_H__
#define __TIMEZONE_H__


class BString;
namespace icu_44 {
	class TimeZone;
}


class BTimeZone {
	public:
					BTimeZone(const char* zoneCode);
					~BTimeZone();

		void		GetName(BString& name);
		void		GetCode(char* buffer, int size);
		int			OffsetFromGMT();

	private:
		icu_44::TimeZone*	fICUTimeZone;
};


#endif
