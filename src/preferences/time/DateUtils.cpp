#include "DateUtils.h"
#include "math.h"

#include <time.h>

bool
isLeapYear(const int year)
{
	int realYear = year + 1900;
	return ((realYear % 400 == 0)||(realYear % 4 == 0 && realYear % 100 != 0));
}


int
getDaysInMonth(const int month, const int year)
{
	if (month == 1 && isLeapYear(year)) {
		return 29;
	}
	
	static const int DaysinMonth[12] = 
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	return DaysinMonth[month];
}


int
getFirstDay(const int month, const int year)
{
	struct tm tm;
	time_t currentTime = time(NULL);
	localtime_r(&currentTime, &tm);
	
	// TODO: review this.
	// We rely on the fact that tm.tm_wday is calculated
	// starting from the following parameters
	tm.tm_mon = month;
	tm.tm_year = year;
	tm.tm_mday = 1;
	
	currentTime = mktime(&tm);
	localtime_r(&currentTime, &tm);
		
	return tm.tm_wday;
}
