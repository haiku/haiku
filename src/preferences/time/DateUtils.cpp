#include "DateUtils.h"
#include "math.h"

bool
isLeapYear(const int year)
{
	if ((year % 400 == 0)||(year % 4 == 0 && year % 100 == 0))
		return true;
	else
		return false;
}


int
getDaysInMonth(const int month, const int year)
{
	if (month == 2 && isLeapYear(year)) {
		return 29;
	}
	
	static const int DaysinMonth[12] = 
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	return DaysinMonth[month];
}


int
getFirstDay(const int month, const int year)
{
	int l_century = year/100;
	int l_decade = year%100;
	int l_month = (month +10)%12;
	int l_day = 1;
	
	if (l_month == 1 || l_month == 2) {
		if (l_decade == 0) {
			l_decade = 99;
			l_century -= 1;
		} else
		 l_decade-= 1;
	}
	
	float tmp = (l_day +(floor(((13 *l_month) -1)/5)) 
		+l_decade +(floor(l_decade /4)) 
		+(floor(l_century/4)) -(2 *l_century));
	int result = static_cast<int>(tmp)%7;
	
	if (result < 0)
		result += 7;
		
	return result;
}
