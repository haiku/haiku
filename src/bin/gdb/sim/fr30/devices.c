// OBSOLETE /* fr30 device support
// OBSOLETE    Copyright (C) 1998, 1999 Free Software Foundation, Inc.
// OBSOLETE    Contributed by Cygnus Solutions.
// OBSOLETE 
// OBSOLETE This file is part of the GNU simulators.
// OBSOLETE 
// OBSOLETE This program is free software; you can redistribute it and/or modify
// OBSOLETE it under the terms of the GNU General Public License as published by
// OBSOLETE the Free Software Foundation; either version 2, or (at your option)
// OBSOLETE any later version.
// OBSOLETE 
// OBSOLETE This program is distributed in the hope that it will be useful,
// OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE You should have received a copy of the GNU General Public License along
// OBSOLETE with this program; if not, write to the Free Software Foundation, Inc.,
// OBSOLETE 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE /* ??? All of this is just to get something going.  wip!  */
// OBSOLETE 
// OBSOLETE #include "sim-main.h"
// OBSOLETE 
// OBSOLETE #ifdef HAVE_DV_SOCKSER
// OBSOLETE #include "dv-sockser.h"
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE device fr30_devices;
// OBSOLETE 
// OBSOLETE int
// OBSOLETE device_io_read_buffer (device *me, void *source, int space,
// OBSOLETE 		       address_word addr, unsigned nr_bytes,
// OBSOLETE 		       SIM_DESC sd, SIM_CPU *cpu, sim_cia cia)
// OBSOLETE {
// OBSOLETE   if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
// OBSOLETE     return nr_bytes;
// OBSOLETE 
// OBSOLETE #ifdef HAVE_DV_SOCKSER
// OBSOLETE   if (addr == UART_INCHAR_ADDR)
// OBSOLETE     {
// OBSOLETE       int c = dv_sockser_read (sd);
// OBSOLETE       if (c == -1)
// OBSOLETE 	return 0;
// OBSOLETE       *(char *) source = c;
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE   if (addr == UART_STATUS_ADDR)
// OBSOLETE     {
// OBSOLETE       int status = dv_sockser_status (sd);
// OBSOLETE       unsigned char *p = source;
// OBSOLETE       p[0] = 0;
// OBSOLETE       p[1] = (((status & DV_SOCKSER_INPUT_EMPTY)
// OBSOLETE #ifdef UART_INPUT_READY0
// OBSOLETE 	       ? UART_INPUT_READY : 0)
// OBSOLETE #else
// OBSOLETE 	       ? 0 : UART_INPUT_READY)
// OBSOLETE #endif
// OBSOLETE 	      + ((status & DV_SOCKSER_OUTPUT_EMPTY) ? UART_OUTPUT_READY : 0));
// OBSOLETE       return 2;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   return nr_bytes;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE device_io_write_buffer (device *me, const void *source, int space,
// OBSOLETE 			address_word addr, unsigned nr_bytes,
// OBSOLETE 			SIM_DESC sd, SIM_CPU *cpu, sim_cia cia)
// OBSOLETE {
// OBSOLETE #if WITH_SCACHE
// OBSOLETE   if (addr == MCCR_ADDR)
// OBSOLETE     {
// OBSOLETE       if ((*(const char *) source & MCCR_CP) != 0)
// OBSOLETE 	scache_flush (sd);
// OBSOLETE       return nr_bytes;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
// OBSOLETE     return nr_bytes;
// OBSOLETE 
// OBSOLETE #if HAVE_DV_SOCKSER
// OBSOLETE   if (addr == UART_OUTCHAR_ADDR)
// OBSOLETE     {
// OBSOLETE       int rc = dv_sockser_write (sd, *(char *) source);
// OBSOLETE       return rc == 1;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   return nr_bytes;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE device_error (device *me, char *message, ...)
// OBSOLETE {
// OBSOLETE }
