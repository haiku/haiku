/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.

   - PPC Port: Andreas Drewke (andreas_dr@gmx.de)
   - Voodoo3Driver 0.02 (c) by Carwyn Jones (2002)
   
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

area_id iobase_area;
vuint32 *iobase;

#ifdef DEBUG
void dpf (const char * format, ...)
{
    char buffer[4096] = "VOODOO: ";
    va_list args;
    va_start (args, format);
    vsprintf (buffer + 8, format, args);
    ioctl (fd, VOODOO_DPRINTF, buffer, 0);
    va_end (args);
}
#endif

void outb(uint32 port, uint8 data)
{
	voodoo_port_io io;
	io.port = port;
	io.data8 = data;
	ioctl(fd, VOODOO_OUTB, &io, 0);
	return;
}

void outw(uint32 port, uint16 data)
{
	voodoo_port_io io;
	io.port = port;
	io.data16 = data;
	ioctl(fd, VOODOO_OUTW, &io, 0);
	return;
}

void outl(uint32 port, uint32 data)
{
	voodoo_port_io io;
	io.port = port;
	io.data32 = data;
	ioctl(fd, VOODOO_OUTL, &io, 0);
	return;
}

uint8 inb(uint32 port)
{
	voodoo_port_io io;
	io.port = port;
	ioctl(fd, VOODOO_INB, &io, 0);
	return(io.data8);
}

uint16 inw(uint32 port)
{
	voodoo_port_io io;
	io.port = port;
	ioctl(fd, VOODOO_INW, &io, 0);
	return(io.data16);
}

uint32 inl(uint32 port)
{
	voodoo_port_io io;
	io.port = port;
	ioctl(fd, VOODOO_INL, &io, 0);
	return(io.data32);
}
