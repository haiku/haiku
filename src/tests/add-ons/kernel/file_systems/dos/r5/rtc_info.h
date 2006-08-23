/* ++++++++++
	FILE:	rtc_info.h
	NAME:	mani
	DATE:	2/1998
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
+++++ */

#ifndef _RTC_INFO_H
#define _RTC_INFO_H

typedef struct {
        uint32          time;
        bool            is_gmt;
        int32           tz_minuteswest;
        int32           tz_dsttime;
} rtc_info;

#define RTC_SETTINGS_FILE	"RTC_time_settings"

extern _IMPEXP_KERNEL status_t get_rtc_info(rtc_info *);

#endif /* _RTC_INFO_H */
