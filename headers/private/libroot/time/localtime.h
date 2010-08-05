/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCALTIME_H
#define _LOCALTIME_H


#include <SupportDefs.h>


namespace BPrivate {


static const char*	skPosixTimeZoneInfoFile = "libroot_timezone_info";
static const int	skTimeZoneInfoNameMax	= 32;

// holds persistent info set by Time preflet
struct time_zone_info {
	char	short_std_name[skTimeZoneInfoNameMax];
	char	short_dst_name[skTimeZoneInfoNameMax];
	uint32	offset_from_gmt;
	bool	uses_daylight_saving;
};


}	// namespace BPrivate


#endif	// _LOCALTIME_H
