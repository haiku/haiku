/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.

   - PPC Port: Andreas Drewke (andreas_dr@gmx.de)
   - Voodoo3Driver 0.02 (c) by Carwyn Jones (2002)
   
*/

#if !defined(GLOBALDATA_H)
#define GLOBALDATA_H

#include <video_overlay.h>
#include <DriverInterface.h>
#include <strings.h>

extern int fd;
extern shared_info *si;
extern area_id shared_info_area;
extern vuint32 *regs;
extern area_id regs_area;
extern area_id iobase_area;
extern vuint32 *iobase;
extern vuint32 *framebuffer;
extern area_id fb_area;
extern display_mode *my_mode_list;
extern area_id my_mode_list_area;
extern int accelerantIsClone;

/* Print debug message through kernel driver. Should move to other location later. */
// #define DEBUG

#ifdef DEBUG
#define ddpf(a) dpf a
extern void dpf (const char * format, ...); 
#else
#define ddpf(a) 
#endif
extern void outb(uint32 port, uint8 data);
extern void outw(uint32 port, uint16 data);
extern void outl(uint32 port, uint32 data);
extern uint8 inb(uint32 port);
extern uint16 inw(uint32 port);
extern uint32 inl(uint32 port);
#endif
