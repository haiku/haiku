/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DATE_TIME_H_
#define _DATE_TIME_H_


#include <String.h>


namespace BPrivate {


enum time_type {
	B_GMT_TIME,
	B_LOCAL_TIME
};


class BTime {
	public:
						BTime();
						BTime(int32 hour, int32 minute, int32 second);
						~BTime();

		bool			IsValid() const;

		static BTime	CurrentTime(time_type type);
		bool			SetTime(int32 hour, int32 minute, int32 second);

		int32			Hour() const;
		int32			Minute() const;
		int32			Second() const;

	private:
		int32			fHour;
		int32			fMinute;
		int32			fSecond;
};


class BDate {
	public:
						BDate();
						BDate(int32 year, int32 month, int32 day);
						~BDate();

		bool			IsValid() const;

		static BDate	CurrentDate(time_type type);
		bool			SetDate(int32 year, int32 month, int32 day);
		
		void			AddDays(int32 days);

		int32			Day() const;
		int32			Year() const;
		int32			Month() const;

		int32			DayOfWeek() const;
		int32			DayOfYear() const;

		int32			WeekNumber() const;
		bool			IsLeapYear(int32 year) const;

		int32			DaysInYear() const;
		int32			DaysInMonth() const;

		BString			ShortDayName(int32 day) const;
		BString			ShortMonthName(int32 month) const;

		BString			LongDayName(int32 day) const;
		BString			LongMonthName(int32 month) const;

	private:
		int32			fDay;
		int32			fYear;
		int32			fMonth;
};


class BDateTime {
	public:
							BDateTime(const BDate &date, const BTime &time);
							~BDateTime();

		bool				IsValid() const;

		static BDateTime	CurrentDateTime(time_type type);
		void				SetDateTime(const BDate &date, const BTime &time);

		BDate				Date() const;
		void				SetDate(const BDate &date);

		BTime				Time() const;
		void				SetTime(const BTime &time);

		uint32				Time_t() const;

	private:
		BDate				fDate;
		BTime				fTime;
};


}	// namespace BPrivate


#endif	// _DATE_TIME_H_
