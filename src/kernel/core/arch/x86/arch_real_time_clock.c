/* 
** Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <arch/real_time_clock.h>
#include <arch/cpu.h>


#define CMOS_ADDR_PORT 0x70
#define CMOS_DATA_PORT 0x71
#define BASE_YEAR 1970
#define SECONDS_31 2678400
#define SECONDS_30 2592000
#define SECONDS_28 2419200
#define SECONDS_DAY 86400

typedef struct	{
	uint8 second;
	uint8 minute;
	uint8 hour;
	uint8 day;
	uint8 month;
	uint8 year;
	uint8 century;
} cmos_time;

uint32 secs_per_month[12] = {SECONDS_31, SECONDS_28, SECONDS_31, SECONDS_30, 
	SECONDS_31, SECONDS_30, SECONDS_31, SECONDS_31, SECONDS_30, SECONDS_31, SECONDS_30,
	SECONDS_31};


static uint32
bcd_to_int(uint8 bcd)
{
	uint32 numl;
	uint32 numh;

	numl = bcd & 0x0f;
	numh = (bcd & 0xf0) >> 4;

	return numh * 10 + numl;
}


static uint8
int_to_bcd(uint32 number)
{
	uint8 low;
	uint8 high;

	if (number > 99)
		return 0;

	high = number / 10;
	low = number % 10;

	return (high << 4) | low;
}


static int
leap_year(uint32 year)
{
	if (year % 400 == 0)
		return 1;

	if (year % 100 == 0)
		return 0;

	if (year % 4 == 0)
		return 1;

	return 0;
}


static int
same_time(const cmos_time *time1, const cmos_time *time2)
{
	return time1->second == time2->second
		&& time1->minute == time2->minute
		&& time1->hour == time2->hour
		&& time1->day == time2->day
		&& time1->month == time2->month
		&& time1->year == time2->year
		&& time1->century == time2->century;
}


static uint8
cmos_read(uint8 addr)
{
	int wait_time;

	wait_time = 10000;

	// Wait until bit 7 of Status Register A (indicating whether or not an update is in
	// progress) is clear if we are reading one of the clock data registers...
	if (addr < 0x0a) {
		out8(0x0a, CMOS_ADDR_PORT);
		while ((in8(CMOS_DATA_PORT) & 0x80) && --wait_time);
	}

	// then read the value.
	out8(addr, CMOS_ADDR_PORT);
	return in8(CMOS_DATA_PORT);
}


static void
cmos_write(uint8 addr, uint8 data)
{
	out8(addr, CMOS_ADDR_PORT);
	out8(data, CMOS_DATA_PORT);
}


static void
set_24_hour_mode(void)
{
	uint8 status_b;

	status_b = cmos_read(0x0b);
	status_b |= 0x02;
	cmos_write(0x0b, status_b);
}


static void
read_cmos_clock(cmos_time *cmos)
{
	set_24_hour_mode();

	cmos->century = cmos_read(0x32);
	cmos->year = cmos_read(0x09);
	cmos->month = cmos_read(0x08);
	cmos->day = cmos_read(0x07);
	cmos->hour = cmos_read(0x04);
	cmos->minute = cmos_read(0x02);
	cmos->second = cmos_read(0x00);
}


static void
write_cmos_clock(cmos_time *cmos)
{
	set_24_hour_mode();

	cmos_write(0x32, cmos->century);
	cmos_write(0x09, cmos->year);
	cmos_write(0x08, cmos->month);
	cmos_write(0x07, cmos->day);
	cmos_write(0x04, cmos->hour);
	cmos_write(0x02, cmos->minute);
	cmos_write(0x00, cmos->second);
}


static inline uint32
secs_this_year(uint32 year)
{
	if (leap_year(year))
		return 31622400;
	
	return 31536000;
}


static uint32
cmos_to_secs(const cmos_time *cmos)
{
	uint32 wholeYear;
	uint32 time = 0;
	int i;

	wholeYear = bcd_to_int(cmos->century) * 100 + bcd_to_int(cmos->year);

	// Add up the seconds from all years since 1970 that have elapsed.
	for (i = BASE_YEAR; i < wholeYear; ++i) {
		time += secs_this_year(i);
	}

	// Add up the seconds from all months passed this year.
	for (i = 0; i < bcd_to_int(cmos->month) - 1 && i < 12; ++i)
		time += secs_per_month[i];

	// Add up the seconds from all days passed this month.
	if (leap_year(wholeYear) && bcd_to_int(cmos->month) > 2)
		time += SECONDS_DAY;

	time += (bcd_to_int(cmos->day) - 1) * SECONDS_DAY;
	time += bcd_to_int(cmos->hour) * 3600;
	time += bcd_to_int(cmos->minute) * 60;
	time += bcd_to_int(cmos->second);

	return time;
}


static void
secs_to_cmos(uint32 seconds, cmos_time *cmos)
{
	uint32 wholeYear = BASE_YEAR;
	uint32 secsThisYear;
	bool keepLooping;
	bool isLeapYear;
	int i;
	int temp;
	int month;

	keepLooping = 1;

	// Determine the current year by starting at 1970 and incrementing whole_year as long as
	// we can keep subtracting secs_this_year from seconds.
	while (keepLooping) {
		secsThisYear = secs_this_year(wholeYear);

		if (seconds >= secsThisYear) {
			seconds -= secsThisYear;
			++wholeYear;
		} else
			keepLooping = false;
	}

	cmos->century = int_to_bcd(wholeYear / 100);
	cmos->year = int_to_bcd(wholeYear % 100);

	// Determine the current month
	month = 1;
	isLeapYear = leap_year(wholeYear);
	do {
		temp = seconds - secs_per_month[month - 1];

		if (isLeapYear && month == 2)
			temp -= SECONDS_DAY;

		if (temp >= 0) {
			seconds = temp;
			++month;
		}
	} while (temp >= 0 && month < 13);

	cmos->month = int_to_bcd(month);

	cmos->day = int_to_bcd(seconds / SECONDS_DAY + 1);
	seconds = seconds % SECONDS_DAY;

	cmos->hour = int_to_bcd(seconds / 3600);
	seconds = seconds % 3600;

	cmos->minute = int_to_bcd(seconds / 60);
	seconds = seconds % 60;

	cmos->second = int_to_bcd(seconds);
}


uint32
arch_rtc_get_hw_time(void)
{
	int waitTime;
	cmos_time cmos1;
	cmos_time cmos2;

	waitTime = 1000;

	// We will read the clock twice and make sure both reads are equal.  This will prevent
	// problems that would occur if the clock is read during an update (e.g. if we read the hour
	// at 8:59:59, the clock gets changed, and then we read the minute and second, we would
	// be off by a whole hour)
	do {
		read_cmos_clock(&cmos1);
		read_cmos_clock(&cmos2);
	} while (!same_time(&cmos1, &cmos2) && --waitTime);

	// Convert the CMOS data to seconds since 1970.
	return cmos_to_secs(&cmos1);
}


void
arch_rtc_set_hw_time(uint32 seconds)
{
	cmos_time cmos;

	secs_to_cmos(seconds, &cmos);
	write_cmos_clock(&cmos);
}	
