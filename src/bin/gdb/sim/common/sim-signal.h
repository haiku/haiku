/* Simulator signal support
   Copyright (C) 1997 Free Software Foundation, Inc.
   Contributed by Cygnus Support

This file is part of the GNU Simulators.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef SIM_SIGNAL_H
#define SIM_SIGNAL_H

/* Signals we use.
   This provides a layer between our values and host/target values.  */

typedef enum {
  SIM_SIGNONE = 64,
  /* illegal insn */
  SIM_SIGILL,
  /* breakpoint */
  SIM_SIGTRAP,
  /* misaligned memory access */
  SIM_SIGBUS,
  /* tried to read/write memory that's not readable/writable */
  SIM_SIGSEGV,
  /* cpu limit exceeded */
  SIM_SIGXCPU,
  /* simulation interrupted (sim_stop called) */
  SIM_SIGINT,
  /* Floating point or integer divide */
  SIM_SIGFPE,
  /* simulation aborted */
  SIM_SIGABRT
} SIM_SIGNAL;

int sim_signal_to_host (SIM_DESC sd, SIM_SIGNAL);

#endif /* SIM_SIGNAL_H */
