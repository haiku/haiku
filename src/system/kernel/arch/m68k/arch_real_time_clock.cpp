/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights reserved.
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2003, Jeff Ward, jeff@r2d2.stcloudstate.edu. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include <arch/real_time_clock.h>

#include <arch_platform.h>
#include <boot/kernel_args.h>
#include <real_time_clock.h>
#include <real_time_data.h>
#include <smp.h>

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
	return M68KPlatform::Default()->ReadRTCReg(addr);
}


static void
cmos_write(uint8 addr, uint8 data)
{
	M68KPlatform::Default()->WriteRTCReg(addr, data);
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



static spinlock sSetArchDataLock;

status_t
arch_rtc_init(kernel_args *args, struct real_time_data *data)
{
	// init the platform RTC service
	status_t error = M68KPlatform::Default()->InitRTC(args, data);
	if (error != B_OK)
		return error;

	// init the arch specific part of the real_time_data
	data->arch_data.data[0].system_time_offset = 0;
	// cvFactor = 2^32 * 1000000 / tbFreq
	// => (tb * cvFactor) >> 32 = (tb * 2^32 * 1000000 / tbFreq) >> 32
	//    = tb / tbFreq * 1000000 = time in us
	data->arch_data.system_time_conversion_factor
		= uint32((uint64(1) << 32) * 1000000
			/ args->arch_args.time_base_frequency);
	data->arch_data.version = 0;

	// init spinlock
	sSetArchDataLock = 0;

	// init system_time() conversion factor
	__m68k_setup_system_time(&data->arch_data.system_time_conversion_factor);

	return B_OK;
}


uint32
arch_rtc_get_hw_time(void)
{
	return M68KPlatform::Default()->GetHardwareRTC();
}


void
arch_rtc_set_hw_time(uint32 seconds)
{
	M68KPlatform::Default()->SetHardwareRTC(seconds);
}


void
arch_rtc_set_system_time_offset(struct real_time_data *data, bigtime_t offset)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&sSetArchDataLock);

	int32 version = data->arch_data.version + 1;
	data->arch_data.data[version % 2].system_time_offset = offset;
	data->arch_data.version = version;

	release_spinlock(&sSetArchDataLock);
	restore_interrupts(state);
}


bigtime_t
arch_rtc_get_system_time_offset(struct real_time_data *data)
{
	int32 version;
	bigtime_t offset;
	do {
		version = data->arch_data.version;
		offset = data->arch_data.data[version % 2].system_time_offset;
	} while (version != data->arch_data.version);

	return offset;
}
