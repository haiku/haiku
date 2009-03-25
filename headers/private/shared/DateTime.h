/*
 * Copyright 2007-2009, Haiku, Inc. All Rights Reserved.
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


enum diff_type {
	B_HOURS_DIFF,
	B_MINUTES_DIFF,
	B_SECONDS_DIFF,
	B_MILLISECONDS_DIFF,
	B_MICROSECONDS_DIFF
};


class BTime {
	public:
						BTime();
						BTime(int32 hour, int32 minute, int32 second,
							int32 microsecond = 0);
						~BTime();

		bool			IsValid() const;
		bool			IsValid(const BTime& time) const;
		bool			IsValid(int32 hour, int32 minute, int32 second,
							int32 microsecond = 0) const;

		static BTime	CurrentTime(time_type type);

		BTime			Time() const;
		bool			SetTime(const BTime& time);
		bool			SetTime(int32 hour, int32 minute, int32 second,
							int32 microsecond = 0);

		void			AddHours(int32 hours);
		void			AddMinutes(int32 minutes);
		void			AddSeconds(int32 seconds);
		void			AddMilliseconds(int32 milliseconds);
		void			AddMicroseconds(int32 microseconds);

		int32			Hour() const;
		int32			Minute() const;
		int32			Second() const;
		int32			Millisecond() const;
		int32			Microsecond() const;
		bigtime_t		Difference(const BTime& time, diff_type type) const;

		bool			operator!=(const BTime& time) const;
		bool			operator==(const BTime& time) const;

		bool			operator<(const BTime& time) const;
		bool			operator<=(const BTime& time) const;

		bool			operator>(const BTime& time) const;
		bool			operator>=(const BTime& time) const;

	private:
		bigtime_t		_Microseconds() const;
		void			_AddMicroseconds(bigtime_t microseconds);
		bool			_SetTime(bigtime_t hour, bigtime_t minute, bigtime_t second,
							bigtime_t microsecond);

	private:
		bigtime_t		fMicroseconds;
};


class BDate {
	public:
						BDate();
						BDate(int32 year, int32 month, int32 day);
						~BDate();

		bool			IsValid() const;
		bool			IsValid(const BDate& date) const;
		bool			IsValid(int32 year, int32 month, int32 day) const;

		static BDate	CurrentDate(time_type type);

		BDate			Date() const;
		bool			SetDate(const BDate& date);

		bool			SetDate(int32 year, int32 month, int32 day);
		void			GetDate(int32* year, int32* month, int32* day);

		void			AddDays(int32 days);
		void			AddYears(int32 years);
		void			AddMonths(int32 months);

		int32			Day() const;
		int32			Year() const;
		int32			Month() const;
		int32			Difference(const BDate& date) const;

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

		int32			DateToJulianDay() const;
		static BDate	JulianDayToDate(int32 julianDay);

		bool			operator!=(const BDate& date) const;
		bool			operator==(const BDate& date) const;

		bool			operator<(const BDate& date) const;
		bool			operator<=(const BDate& date) const;

		bool			operator>(const BDate& date) const;
		bool			operator>=(const BDate& date) const;

	private:
		int32			_DaysInMonth(int32 year, int32 month) const;
		bool			_SetDate(int32 year, int32 month, int32 day);
		int32			_DateToJulianDay(int32 year, int32 month, int32 day) const;

	private:
		int32			fDay;
		int32			fYear;
		int32			fMonth;
};


class BDateTime {
	public:
							BDateTime();
							BDateTime(const BDate &date, const BTime &time);
							~BDateTime();

		bool				IsValid() const;

		static BDateTime	CurrentDateTime(time_type type);
		void				SetDateTime(const BDate &date, const BTime &time);

		BDate				Date() const;
		void				SetDate(const BDate &date);

		BTime				Time() const;
		void				SetTime(const BTime &time);

		static	uint32		Time_t(time_type type);

	private:
		BDate				fDate;
		BTime				fTime;
};


}	// namespace BPrivate


#endif	// _DATE_TIME_H_
