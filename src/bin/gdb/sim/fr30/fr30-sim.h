// OBSOLETE /* collection of junk waiting time to sort out
// OBSOLETE    Copyright (C) 1998, 1999 Free Software Foundation, Inc.
// OBSOLETE    Contributed by Cygnus Solutions.
// OBSOLETE 
// OBSOLETE This file is part of the GNU Simulators.
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
// OBSOLETE #ifndef FR30_SIM_H
// OBSOLETE #define FR30_SIM_H
// OBSOLETE 
// OBSOLETE /* gdb register numbers */
// OBSOLETE #define PC_REGNUM	16
// OBSOLETE #define PS_REGNUM	17
// OBSOLETE #define TBR_REGNUM	18
// OBSOLETE #define RP_REGNUM	19
// OBSOLETE #define SSP_REGNUM	20
// OBSOLETE #define USP_REGNUM	21
// OBSOLETE #define MDH_REGNUM	22
// OBSOLETE #define MDL_REGNUM	23
// OBSOLETE 
// OBSOLETE extern BI fr30bf_h_sbit_get_handler (SIM_CPU *);
// OBSOLETE extern void fr30bf_h_sbit_set_handler (SIM_CPU *, BI);
// OBSOLETE 
// OBSOLETE extern UQI fr30bf_h_ccr_get_handler (SIM_CPU *);
// OBSOLETE extern void fr30bf_h_ccr_set_handler (SIM_CPU *, UQI);
// OBSOLETE 
// OBSOLETE extern UQI fr30bf_h_scr_get_handler (SIM_CPU *);
// OBSOLETE extern void fr30bf_h_scr_set_handler (SIM_CPU *, UQI);
// OBSOLETE 
// OBSOLETE extern UQI fr30bf_h_ilm_get_handler (SIM_CPU *);
// OBSOLETE extern void fr30bf_h_ilm_set_handler (SIM_CPU *, UQI);
// OBSOLETE 
// OBSOLETE extern USI fr30bf_h_ps_get_handler (SIM_CPU *);
// OBSOLETE extern void fr30bf_h_ps_set_handler (SIM_CPU *, USI);
// OBSOLETE 
// OBSOLETE extern SI fr30bf_h_dr_get_handler (SIM_CPU *, UINT);
// OBSOLETE extern void fr30bf_h_dr_set_handler (SIM_CPU *, UINT, SI);
// OBSOLETE 
// OBSOLETE #define GETTWI GETTSI
// OBSOLETE #define SETTWI SETTSI
// OBSOLETE 
// OBSOLETE /* Hardware/device support.
// OBSOLETE    ??? Will eventually want to move device stuff to config files.  */
// OBSOLETE 
// OBSOLETE /* Special purpose traps.  */
// OBSOLETE #define TRAP_SYSCALL	10
// OBSOLETE #define TRAP_BREAKPOINT	9
// OBSOLETE 
// OBSOLETE /* Support for the MCCR register (Cache Control Register) is needed in order
// OBSOLETE    for overlays to work correctly with the scache: cached instructions need
// OBSOLETE    to be flushed when the instruction space is changed at runtime.  */
// OBSOLETE 
// OBSOLETE /* Cache Control Register */
// OBSOLETE #define MCCR_ADDR 0xffffffff
// OBSOLETE #define MCCR_CP 0x80
// OBSOLETE /* not supported */
// OBSOLETE #define MCCR_CM0 2
// OBSOLETE #define MCCR_CM1 1
// OBSOLETE 
// OBSOLETE /* Serial device addresses.  */
// OBSOLETE /* These are the values for the MSA2000 board.
// OBSOLETE    ??? Will eventually need to move this to a config file.  */
// OBSOLETE #define UART_INCHAR_ADDR	0xff004009
// OBSOLETE #define UART_OUTCHAR_ADDR	0xff004007
// OBSOLETE #define UART_STATUS_ADDR	0xff004002
// OBSOLETE 
// OBSOLETE #define UART_INPUT_READY	0x4
// OBSOLETE #define UART_OUTPUT_READY	0x1
// OBSOLETE 
// OBSOLETE /* Start address and length of all device support.  */
// OBSOLETE #define FR30_DEVICE_ADDR	0xff000000
// OBSOLETE #define FR30_DEVICE_LEN		0x00ffffff
// OBSOLETE 
// OBSOLETE /* sim_core_attach device argument.  */
// OBSOLETE extern device fr30_devices;
// OBSOLETE 
// OBSOLETE /* FIXME: Temporary, until device support ready.  */
// OBSOLETE struct _device { int foo; };
// OBSOLETE 
// OBSOLETE /* Handle the trap insn.  */
// OBSOLETE USI fr30_int (SIM_CPU *, PCADDR, int);
// OBSOLETE 
// OBSOLETE #endif /* FR30_SIM_H */
