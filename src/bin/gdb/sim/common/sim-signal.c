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

#include <signal.h>
#include "sim-main.h"

/* Convert SIM_SIGFOO to SIGFOO.
   What to do when the host doesn't have SIGFOO is handled on a case by case
   basis.  Generally, in the case of passing a value back to gdb, we want gdb
   to not think the process has died (so it can be debugged at the point of
   failure).  */

#ifdef _MSC_VER
#ifndef SIGTRAP
#define SIGTRAP 5
#endif
#ifndef SIGBUS
#define SIGBUS 10
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#endif

int
sim_signal_to_host (SIM_DESC sd, SIM_SIGNAL sig)
{
  switch (sig)
    {
    case SIM_SIGINT :
      return SIGINT;

    case SIM_SIGABRT :
      return SIGABRT;

    case SIM_SIGILL :
#ifdef SIGILL
      return SIGILL;
#else
      return SIGSEGV;
#endif

    case SIM_SIGTRAP :
      return SIGTRAP;

    case SIM_SIGBUS :
#ifdef SIGBUS
      return SIGBUS;
#else
      return SIGSEGV;
#endif

    case SIM_SIGSEGV :
      return SIGSEGV;

    case SIM_SIGXCPU :
#ifdef SIGXCPU
      return SIGXCPU;
#endif
      break;

    case SIM_SIGFPE:
#ifdef SIGXCPU
      return SIGFPE;
#endif
      break;

    case SIM_SIGNONE:
      return 0;
      break;
    }

  sim_io_eprintf (sd, "sim_signal_to_host: unknown signal: %d\n", sig);
#ifdef SIGHUP
  return SIGHUP;  /* FIXME: Suggestions?  */
#else
  return 1;
#endif
}
