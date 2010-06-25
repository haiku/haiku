/*
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <arch/real_time_clock.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>

#include <real_time_clock.h>
#include <real_time_data.h>


#define CMOS_ADDR_PORT 0x70
#define CMOS_DATA_PORT 0x71

typedef struct	{
	uint8 second;
	uint8 minute;
	uint8 hour;
	uint8 day;
	uint8 month;
	uint8 year;
	uint8 century;
} cmos_time;


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
	int waitTime = 10000;

	// Wait until bit 7 of Status Register A (indicating whether or not an update is in
	// progress) is clear if we are reading one of the clock data registers...
	if (addr < 0x0a) {
		out8(0x0a, CMOS_ADDR_PORT);
		while ((in8(CMOS_DATA_PORT) & 0x80) && --waitTime);
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


static uint32
cmos_to_secs(const cmos_time *cmos)
{
	struct tm t;
	t.tm_year = bcd_to_int(cmos->century) * 100 + bcd_to_int(cmos->year)
		- RTC_EPOCH_BASE_YEAR;
	t.tm_mon = bcd_to_int(cmos->month) - 1;
	t.tm_mday = bcd_to_int(cmos->day);
	t.tm_hour = bcd_to_int(cmos->hour);
	t.tm_min = bcd_to_int(cmos->minute);
	t.tm_sec = bcd_to_int(cmos->second);

	return rtc_tm_to_secs(&t);
}


static void
secs_to_cmos(uint32 seconds, cmos_time *cmos)
{
	int wholeYear;

	struct tm t;
	rtc_secs_to_tm(seconds, &t);

	wholeYear = t.tm_year + RTC_EPOCH_BASE_YEAR;

	cmos->century = int_to_bcd(wholeYear / 100);
	cmos->year = int_to_bcd(wholeYear % 100);
	cmos->month = int_to_bcd(t.tm_mon + 1);
	cmos->day = int_to_bcd(t.tm_mday);
	cmos->hour = int_to_bcd(t.tm_hour);
	cmos->minute = int_to_bcd(t.tm_min);
	cmos->second = int_to_bcd(t.tm_sec);
}


//	#pragma mark -


status_t
arch_rtc_init(struct kernel_args *args, struct real_time_data *data)
{
	data->arch_data.system_time_conversion_factor
		= args->arch_args.system_time_cv_factor;
	return B_OK;
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


void
arch_rtc_set_system_time_offset(struct real_time_data *data, bigtime_t offset)
{
	atomic_set64(&data->arch_data.system_time_offset, offset);
}


bigtime_t
arch_rtc_get_system_time_offset(struct real_time_data *data)
{
	return atomic_get64(&data->arch_data.system_time_offset);
}
