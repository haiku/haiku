/* gdb->simulator interface.
   Copyright (C) 1992, 1993, 1994, 1997 Free Software Foundation, Inc.

This file is part of Z8KSIM

Z8KSIM is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

Z8KSIM is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Z8KZIM; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "ansidecl.h"
#include "sim.h"
#include "tm.h"
#include "signal.h"
#include "bfd.h"
#include "gdb/callback.h"
#include "gdb/remote-sim.h"

#ifndef NULL
#define NULL 0
#endif

host_callback *z8k_callback;

static SIM_OPEN_KIND sim_kind;
static char *myname;

void
sim_size (n)
     int n;
{
  /* Size is fixed.  */
}

int
sim_store_register (sd, regno, value, length)
     SIM_DESC sd;
     int regno;
     unsigned char *value;
     int length;
{
  /* FIXME: Review the computation of regval.  */
  int regval = (value[0] << 24) | (value[1] << 16) | (value[2] << 8) | value[3];

  tm_store_register (regno, regval);
  return -1;
}

int
sim_fetch_register (sd, regno, buf, length)
     SIM_DESC sd;
     int regno;
     unsigned char *buf;
     int length;
{
  tm_fetch_register (regno, buf);
  return -1;
}

int
sim_write (sd, where, what, howmuch)
     SIM_DESC sd;
     SIM_ADDR where;
     unsigned char *what;
     int howmuch;
{
  int i;

  for (i = 0; i < howmuch; i++)
    tm_write_byte (where + i, what[i]);
  return howmuch;
}

int
sim_read (sd, where, what, howmuch)
     SIM_DESC sd;
     SIM_ADDR where;
     unsigned char *what;
     int howmuch;
{
  int i;

  for (i = 0; i < howmuch; i++)
    what[i] = tm_read_byte (where + i);
  return howmuch;
}

static void 
control_c (sig, code, scp, addr)
     int sig;
     int code;
     char *scp;
     char *addr;
{
  tm_exception (SIM_INTERRUPT);
}

int
sim_stop (sd)
     SIM_DESC sd;
{
  tm_exception (SIM_INTERRUPT);
  return 1;
}

void
sim_resume (sd, step, sig)
     SIM_DESC sd;
     int step;
     int sig;
{
  void (*prev) ();

  prev = signal (SIGINT, control_c);
  tm_resume (step);
  signal (SIGINT, prev);
}

void
sim_stop_reason (sd, reason, sigrc)
     SIM_DESC sd;
     enum sim_stop *reason;
     int *sigrc;
{
  switch (tm_signal ())
    {
    case SIM_DIV_ZERO:
      *sigrc = SIGFPE;
      break;
    case SIM_INTERRUPT:
      *sigrc = SIGINT;
      break;
    case SIM_BAD_INST:
      *sigrc = SIGILL;
      break;
    case SIM_BREAKPOINT:
      *sigrc = SIGTRAP;
      break;
    case SIM_SINGLE_STEP:
      *sigrc = SIGTRAP;
      break;
    case SIM_BAD_SYSCALL:
      *sigrc = SIGILL;
      break;
    case SIM_BAD_ALIGN:
      *sigrc = SIGSEGV;
      break;
    case SIM_DONE:
      {
	sim_state_type x;
	tm_state (&x);
	*sigrc = x.regs[2].word & 255;
	*reason = sim_exited;
	return;
      }
    default:
      abort ();
    }
  *reason = sim_stopped;
}

void
sim_info (sd, verbose)
     SIM_DESC sd;
     int verbose;
{
  sim_state_type x;

  tm_state (&x);
  tm_info_print (&x);
}

SIM_DESC
sim_open (kind, cb, abfd, argv)
     SIM_OPEN_KIND kind;
     host_callback *cb;
     struct bfd *abfd;
     char **argv;
{
  /* FIXME: The code in sim_load that determines the exact z8k arch
     should be moved to here */

  sim_kind = kind;
  myname = argv[0];
  z8k_callback = cb;

  /* fudge our descriptor for now */
  return (SIM_DESC) 1;
}

void
sim_close (sd, quitting)
     SIM_DESC sd;
     int quitting;
{
  /* nothing to do */
}

SIM_RC
sim_load (sd, prog, abfd, from_tty)
     SIM_DESC sd;
     char *prog;
     bfd *abfd;
     int from_tty;
{
  extern bfd *sim_load_file (); /* ??? Don't know where this should live.  */
  bfd *prog_bfd;

  /* FIXME: The code determining the type of z9k processor should be
     moved from here to sim_open. */

  prog_bfd = sim_load_file (sd, myname, z8k_callback, prog, abfd,
			    sim_kind == SIM_OPEN_DEBUG,
			    0, sim_write);
  if (prog_bfd == NULL)
    return SIM_RC_FAIL;
  if (bfd_get_mach (prog_bfd) == bfd_mach_z8001)
    {
      extern int sim_z8001_mode;
      sim_z8001_mode = 1;
    }
  /* Close the bfd if we opened it.  */
  if (abfd == NULL)
    bfd_close (prog_bfd);
  return SIM_RC_OK;
}

SIM_RC
sim_create_inferior (sd, abfd, argv, env)
     SIM_DESC sd;
     struct bfd *abfd;
     char **argv;
     char **env;
{
  if (abfd != NULL)
    tm_store_register (REG_PC, bfd_get_start_address (abfd));
  else
    tm_store_register (REG_PC, 0);
  return SIM_RC_OK;
}

void
sim_do_command (sd, cmd)
     SIM_DESC sd;
     char *cmd;
{
}

void
sim_set_callbacks (ptr)
     host_callback *ptr;
{
  z8k_callback = ptr;
}
