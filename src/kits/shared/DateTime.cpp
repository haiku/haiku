/*
 * Copyright 2007-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun <host.haiku@gmx.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <DateTime.h>


#include <time.h>
#include <sys/time.h>


namespace BPrivate {


const int32			kSecondsPerMinute			= 60;

const int32			kHoursPerDay				= 24;
const int32			kMinutesPerDay				= 1440;
const int32			kSecondsPerDay				= 86400;
const int32			kMillisecondsPerDay			= 86400000;

const bigtime_t		kMicrosecondsPerSecond		= 1000000LL;
const bigtime_t		kMicrosecondsPerMinute		= 60000000LL;
const bigtime_t		kMicrosecondsPerHour		= 3600000000LL;
const bigtime_t		kMicrosecondsPerDay			= 86400000000LL;


BTime::BTime()
	: fMicroseconds(-1)
{
}


BTime::BTime(int32 hour, int32 minute, int32 second, int32 microsecond)
	: fMicroseconds(-1)
{
	_SetTime(hour, minute, second, microsecond);
}


BTime::~BTime()
{
}


bool
BTime::IsValid() const
{
	return fMicroseconds > -1 && fMicroseconds < kMicrosecondsPerDay;
}


bool
BTime::IsValid(const BTime& time) const
{
	return time.fMicroseconds > -1 && time.fMicroseconds < kMicrosecondsPerDay;
}


bool
BTime::IsValid(int32 hour, int32 minute, int32 second, int32 microsecond) const
{
	return BTime(hour, minute, second, microsecond).IsValid();
}


BTime
BTime::CurrentTime(time_type type)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0) {
		// gettimeofday failed?
		time(&tv.tv_sec);
	}

	struct tm result;
	struct tm* timeinfo;
	if (type == B_GMT_TIME)
		timeinfo = gmtime_r(&tv.tv_sec, &result);
	else
		timeinfo = localtime_r(&tv.tv_sec, &result);

	int32 sec = timeinfo->tm_sec;
	return BTime(timeinfo->tm_hour, timeinfo->tm_min, (sec > 59) ? 59 : sec,
		tv.tv_usec);
}


BTime
BTime::Time() const
{
	BTime time;
	time.fMicroseconds = fMicroseconds;
	return time;
}


bool
BTime::SetTime(const BTime& time)
{
	fMicroseconds = time.fMicroseconds;
	return IsValid();
}


bool
BTime::SetTime(int32 hour, int32 minute, int32 second, int32 microsecond)
{
	return _SetTime(hour, minute, second, microsecond);
}


void
BTime::AddHours(int32 hours)
{
	_AddMicroseconds(bigtime_t(hours % kHoursPerDay) * kMicrosecondsPerHour);
}


void
BTime::AddMinutes(int32 minutes)
{
	_AddMicroseconds(bigtime_t(minutes % kMinutesPerDay) *
		kMicrosecondsPerMinute);
}


void
BTime::AddSeconds(int32 seconds)
{
	_AddMicroseconds(bigtime_t(seconds % kSecondsPerDay) *
		kMicrosecondsPerSecond);
}


void
BTime::AddMilliseconds(int32 milliseconds)
{
	_AddMicroseconds(bigtime_t(milliseconds % kMillisecondsPerDay) * 1000);
}


void
BTime::AddMicroseconds(int32 microseconds)
{
	_AddMicroseconds(microseconds);
}


int32
BTime::Hour() const
{
	return int32(_Microseconds() / kMicrosecondsPerHour);
}


int32
BTime::Minute() const
{
	return int32(((_Microseconds() % kMicrosecondsPerHour)) / kMicrosecondsPerMinute);
}


int32
BTime::Second() const
{
	return int32(_Microseconds() / kMicrosecondsPerSecond) % kSecondsPerMinute;
}


int32
BTime::Millisecond() const
{

	return Microsecond() / 1000;
}


int32
BTime::Microsecond() const
{
	return int32(_Microseconds() % 1000000);
}


bigtime_t
BTime::_Microseconds() const
{
	return fMicroseconds == -1 ? 0 : fMicroseconds;
}


bigtime_t
BTime::Difference(const BTime& time, diff_type type) const
{
	bigtime_t diff = time._Microseconds() - _Microseconds();
	switch (type) {
		case B_HOURS_DIFF: {
			diff /= kMicrosecondsPerHour;
		}	break;
		case B_MINUTES_DIFF: {
			diff /= kMicrosecondsPerMinute;
		}	break;
		case B_SECONDS_DIFF: {
			diff /= kMicrosecondsPerSecond;
		}	break;
		case B_MILLISECONDS_DIFF: {
			diff /= 1000;
		}	break;
		case B_MICROSECONDS_DIFF:
		default:	break;
	}
	return diff;
}


bool
BTime::operator!=(const BTime& time) const
{
	return fMicroseconds != time.fMicroseconds;
}


bool
BTime::operator==(const BTime& time) const
{
	return fMicroseconds == time.fMicroseconds;
}


bool
BTime::operator<(const BTime& time) const
{
	return fMicroseconds < time.fMicroseconds;
}


bool
BTime::operator<=(const BTime& time) const
{
	return fMicroseconds <= time.fMicroseconds;
}


bool
BTime::operator>(const BTime& time) const
{
	return fMicroseconds > time.fMicroseconds;
}


bool
BTime::operator>=(const BTime& time) const
{
	return fMicroseconds >= time.fMicroseconds;
}


void
BTime::_AddMicroseconds(bigtime_t microseconds)
{
	bigtime_t count = 0;
	if (microseconds < 0) {
		count = ((kMicrosecondsPerDay - microseconds) / kMicrosecondsPerDay) *
			kMicrosecondsPerDay;
	}
	fMicroseconds = (_Microseconds() + microseconds + count) % kMicrosecondsPerDay;
}


bool
BTime::_SetTime(bigtime_t hour, bigtime_t minute, bigtime_t second,
	bigtime_t microsecond)
{
	fMicroseconds = hour * kMicrosecondsPerHour +
					minute * kMicrosecondsPerMinute +
					second * kMicrosecondsPerSecond +
					microsecond;

	bool isValid = IsValid();
	if (!isValid)
		fMicroseconds = -1;

	return isValid;
}


//	#pragma mark - BDate


BDate::BDate()
	: fDay(-1),
	  fYear(-1),
	  fMonth(-1)
{
}


BDate::BDate(int32 year, int32 month, int32 day)
{
	_SetDate(year, month, day);
}


BDate::~BDate()
{
}


bool
BDate::IsValid() const
{
	return IsValid(fYear, fMonth, fDay);
}


bool
BDate::IsValid(const BDate& date) const
{
	return IsValid(date.fYear, date.fMonth, date.fDay);
}


bool
BDate::IsValid(int32 year, int32 month, int32 day) const
{
	// no year 0 in Julian and we can't handle nothing before 1.1.4713 BC
	if (year == 0 || year < -4713
		|| (year == -4713 && month < 1)
		|| (year == -4713 && month < 1 && day < 1))
		return false;

	// 'missing' days between switch julian - gregorian
	if (year == 1582 && month == 10 && day > 4 && day < 15)
		return false;

	if (month < 1 || month > 12)
		return false;

	if (day < 1 || day > _DaysInMonth(year, month))
		return false;

	return true;
}


BDate
BDate::CurrentDate(time_type type)
{
	time_t timer;
	struct tm result;
	struct tm* timeinfo;

	time(&timer);

	if (type == B_GMT_TIME)
		timeinfo = gmtime_r(&timer, &result);
	else
		timeinfo = localtime_r(&timer, &result);

	return BDate(timeinfo->tm_year + 1900, timeinfo->tm_mon +1, timeinfo->tm_mday);
}


BDate
BDate::Date() const
{
	return BDate(fYear, fMonth, fDay);
}


bool
BDate::SetDate(const BDate& date)
{
	return _SetDate(date.fYear, date.fMonth, date.fDay);
}


bool
BDate::SetDate(int32 year, int32 month, int32 day)
{
	return _SetDate(year, month, day);
}


void
BDate::GetDate(int32* year, int32* month, int32* day)
{
	if (year)
		*year = fYear;

	if (month)
		*month = fMonth;

	if (day)
		*day = fDay;
}


void
BDate::AddDays(int32 days)
{
	*this = JulianDayToDate(DateToJulianDay() + days);
}


void
BDate::AddYears(int32 years)
{
	fYear += years;
	fDay = min_c(fDay, _DaysInMonth(fYear, fMonth));
}


void
BDate::AddMonths(int32 months)
{
	fYear += months / 12;
	fMonth +=  months % 12;

	if (fMonth > 12) {
		fYear++;
		fMonth -= 12;
	} else if (fMonth < 1) {
		fYear--;
		fMonth += 12;
	}

	// 'missing' days between switch julian - gregorian
	if (fYear == 1582 && fMonth == 10 && fDay > 4 && fDay < 15)
		fDay = (months > 0) ? 15 : 4;

	fDay = min_c(fDay, DaysInMonth());
}


int32
BDate::Day() const
{
	return fDay;
}


int32
BDate::Year() const
{
	return fYear;
}


int32
BDate::Month() const
{
	return fMonth;
}


int32
BDate::Difference(const BDate& date) const
{
	return DateToJulianDay() - date.DateToJulianDay();
}


int32
BDate::WeekNumber() const
{
	/*
		This algorithm is taken from:
		Frequently Asked Questions about Calendars
		Version 2.8 Claus Tøndering 15 December 2005

		Note: it will work only within the Gregorian Calendar
	*/

	if (!IsValid() || fYear < 1582
		|| (fYear == 1582 && fMonth < 10)
		|| (fYear == 1582 && fMonth == 10 && fDay < 15))
		return 0;

	int32 a;
	int32 b;
	int32 s;
	int32 e;
	int32 f;

	if (fMonth > 0 && fMonth < 3) {
		a = fYear - 1;
		b = (a / 4) - (a / 100) + (a / 400);
		int32 c = ((a - 1) / 4) - ((a - 1) / 100) + ((a -1) / 400);
		s = b - c;
		e = 0;
		f = fDay - 1 + 31 * (fMonth - 1);
	} else if (fMonth >= 3 && fMonth <= 12) {
		a = fYear;
		b = (a / 4) - (a / 100) + (a / 400);
		int32 c = ((a - 1) / 4) - ((a - 1) / 100) + ((a -1) / 400);
		s = b - c;
		e = s + 1;
		f = fDay + ((153 * (fMonth - 3) + 2) / 5) + 58 + s;
	} else
		return 0;

	int32 g = (a + b) % 7;
	int32 d = (f + g - e) % 7;
	int32 n = f + 3 - d;

	int32 weekNumber;
	if (n < 0)
		weekNumber = 53 - (g -s) / 5;
	else if (n > 364 + s)
		weekNumber = 1;
	else
		weekNumber = n / 7 + 1;

	return weekNumber;
}


int32
BDate::DayOfWeek() const
{
	// http://en.wikipedia.org/wiki/Julian_day#Calculation
	return (DateToJulianDay() % 7) + 1;
}


int32
BDate::DayOfYear() const
{
	return DateToJulianDay() - _DateToJulianDay(fYear, 1, 1) + 1;
}


bool
BDate::IsLeapYear(int32 year) const
{
	if (year < 1582) {
		if (year < 0)
			year++;
		return (year % 4) == 0;
	}
	return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}


int32
BDate::DaysInYear() const
{
	if (!IsValid())
		return 0;

	return IsLeapYear(fYear) ? 366 : 365;
}


int32
BDate::DaysInMonth() const
{
	return _DaysInMonth(fYear, fMonth);
}


BString
BDate::ShortDayName(int32 day) const
{
	if (day < 1 || day > 7)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_wday = day == 7 ? 0 : day;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%a", &tm_struct);

	return BString(buffer);
}


BString
BDate::ShortMonthName(int32 month) const
{
	if (month < 1 || month > 12)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_mon = month -1;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%b", &tm_struct);

	return BString(buffer);
}


BString
BDate::LongDayName(int32 day) const
{
	if (day < 1 || day > 7)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_wday = day == 7 ? 0 : day;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%A", &tm_struct);

	return BString(buffer);
}


BString
BDate::LongMonthName(int32 month) const
{
	if (month < 1 || month > 12)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_mon = month -1;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%B", &tm_struct);

	return BString(buffer);
}


int32
BDate::DateToJulianDay() const
{
	return _DateToJulianDay(fYear, fMonth, fDay);
}


BDate
BDate::JulianDayToDate(int32 julianDay)
{
	BDate date;
	const int32 kGregorianCalendarStart = 2299161;
	if (julianDay >= kGregorianCalendarStart) {
		// http://en.wikipedia.org/wiki/Julian_day#Gregorian_calendar_from_Julian_day_number
		int32 j = julianDay + 32044;
		int32 dg = j % 146097;
		int32 c = (dg / 36524 + 1) * 3 / 4;
		int32 dc = dg - c * 36524;
		int32 db = dc % 1461;
		int32 a = (db / 365 + 1) * 3 / 4;
		int32 da = db - a * 365;
		int32 m = (da * 5 + 308) / 153 - 2;
		date.fYear = ((j / 146097) * 400 + c * 100 + (dc / 1461) * 4 + a) - 4800 +
			(m + 2) / 12;
		date.fMonth = (m + 2) % 12 + 1;
		date.fDay = int32((da - (m + 4) * 153 / 5 + 122) + 1.5);
	} else {
		// http://en.wikipedia.org/wiki/Julian_day#Calculation
		julianDay += 32082;
		int32 d = (4 * julianDay + 3) / 1461;
		int32 e = julianDay - (1461 * d) / 4;
		int32 m = ((5 * e) + 2) / 153;
		date.fDay = e - (153 * m + 2) / 5 + 1;
		date.fMonth = m + 3 - 12 * (m / 10);
		int32 year = d - 4800 + (m / 10);
		if (year <= 0)
			year--;
		date.fYear = year;
	}
	return date;
}


bool
BDate::operator!=(const BDate& date) const
{
	return DateToJulianDay() != date.DateToJulianDay();
}


bool
BDate::operator==(const BDate& date) const
{
	return DateToJulianDay() == date.DateToJulianDay();
}


bool
BDate::operator<(const BDate& date) const
{
	return DateToJulianDay() < date.DateToJulianDay();
}


bool
BDate::operator<=(const BDate& date) const
{
	return DateToJulianDay() <= date.DateToJulianDay();
}


bool
BDate::operator>(const BDate& date) const
{
	return DateToJulianDay() > date.DateToJulianDay();
}


bool
BDate::operator>=(const BDate& date) const
{
	return DateToJulianDay() >= date.DateToJulianDay();
}


bool
BDate::_SetDate(int32 year, int32 month, int32 day)
{
	fDay = -1;
	fYear = -1;
	fMonth = -1;

	bool valid = IsValid(year, month, day);
	if (valid) {
		fDay = day;
		fYear = year;
		fMonth = month;
	}

	return valid;
}


int32
BDate::_DaysInMonth(int32 year, int32 month) const
{
	if (month == 2 && IsLeapYear(year))
		return 29;

	const int32 daysInMonth[12] =
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	return daysInMonth[month -1];
}


int32
BDate::_DateToJulianDay(int32 _year, int32 month, int32 day) const
{
	int32 year = _year;
	if (year < 0) year++;

	int32 a = (14 - month) / 12;
	int32 y = year + 4800 - a;
	int32 m = month + (12 * a) - 3;

	// http://en.wikipedia.org/wiki/Julian_day#Calculation
	if (year > 1582
		|| (year == 1582 && month > 10)
		|| (year == 1582 && month == 10 && day >= 15)) {
		return day + (((153 * m) + 2) / 5) + (365 * y) + (y / 4) -
					(y / 100) + (y / 400) - 32045;
	} else if (year < 1582
		|| (year == 1582 && month < 10)
		|| (year == 1582 && month == 10 && day <= 4)) {
		return day + (((153 * m) + 2) / 5) + (365 * y) + (y / 4) - 32083;
	}

	// http://en.wikipedia.org/wiki/Gregorian_calendar:
	//		The last day of the Julian calendar was Thursday October 4, 1582
	//		and this was followed by the first day of the Gregorian calendar,
	//		Friday October 15, 1582 (the cycle of weekdays was not affected).
	return -1;
}


//	#pragma mark - BDateTime


BDateTime::BDateTime()
	: fDate(BDate()),
	  fTime(BTime())
{
}

BDateTime::BDateTime(const BDate &date, const BTime &time)
	: fDate(date),
	  fTime(time)
{
}


BDateTime::~BDateTime()
{
}


bool
BDateTime::IsValid() const
{
	return fDate.IsValid() && fTime.IsValid();
}


BDateTime
BDateTime::CurrentDateTime(time_type type)
{
	BDate date = BDate::CurrentDate(type);
	BTime time = BTime::CurrentTime(type);

	return BDateTime(date, time);
}


void
BDateTime::SetDateTime(const BDate &date, const BTime &time)
{
	fDate = date;
	fTime = time;
}


BDate
BDateTime::Date() const
{
	return fDate;
}


void
BDateTime::SetDate(const BDate &date)
{
	fDate = date;
}


BTime
BDateTime::Time() const
{
	return fTime;
}


void
BDateTime::SetTime(const BTime &time)
{
	fTime = time;
}


uint32
BDateTime::Time_t() const
{
	BDate date(1970, 1, 1);
	if (date.Difference(fDate) < 0)
		return -1;

	tm tm_struct;

	tm_struct.tm_hour = fTime.Hour();
	tm_struct.tm_min = fTime.Minute();
	tm_struct.tm_sec = fTime.Second();

	tm_struct.tm_year = fDate.Year() - 1900;
	tm_struct.tm_mon = fDate.Month() - 1;
	tm_struct.tm_mday = fDate.Day();

	// set less 0 as we wan't use it
	tm_struct.tm_isdst = -1;

	// return secs_since_jan1_1970 or -1 on error
	return uint32(mktime(&tm_struct));
}

}	//namespace BPrivate
