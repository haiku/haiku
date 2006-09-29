/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef GLOBALDATA_H
#define GLOBALDATA_H


#include "DriverInterface.h"


extern int fd;
extern shared_info *si;
extern area_id shared_info_area;
extern vuint32 *regs;
extern area_id regs_area;
extern vuint32 *framebuffer;
extern area_id fb_area;
extern display_mode *my_mode_list;
extern area_id my_mode_list_area;
extern int accelerantIsClone;

/* Print debug message through kernel driver. Should move to other location later. */
extern void dpf (const char * format, ...);

/* ProposeDisplayMode.c */
extern status_t create_mode_list(void);

/* Cursor.c */
extern void set_cursor_colors(void);

#endif
