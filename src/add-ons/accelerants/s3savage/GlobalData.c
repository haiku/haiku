/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
/*
 * Copyright 1999  Erdi Chen
 */

#include "GlobalData.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/ioctl.h>

int fd;
shared_info *si;
area_id shared_info_area;
vuint32 *regs;
area_id	regs_area;
vuint32 *framebuffer;
area_id fb_area;
display_mode *my_mode_list;
area_id	my_mode_list_area;
int accelerantIsClone;

void dpf (const char * format, ...)
{
    char buffer[4096] = "SAVAGE: ";
    va_list args;
    va_start (args, format);
    vsprintf (buffer + 8, format, args);
    ioctl (fd, SAVAGE_DPRINTF, buffer, 0);
    va_end (args);
}
