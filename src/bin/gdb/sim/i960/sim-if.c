/* Main simulator entry points for the i960.
   Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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

#include "libiberty.h"
#include "bfd.h"
#include "sim-main.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "sim-options.h"
#include "dis-asm.h"

static void free_state (SIM_DESC);

/* Since we don't build the cgen-opcode table, we use the old
   disassembler.  */
static CGEN_DISASSEMBLER i960_disassemble_insn;

/* Records simulator descriptor so utilities like i960_dump_regs can be
   called from gdb.  */
SIM_DESC current_state;

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (kind, callback, abfd, argv)
     SIM_OPEN_KIND kind;
     host_callback *callback;
     struct bfd *abfd;
     char **argv;
{
  char c;
  int i;
  SIM_DESC sd = sim_state_alloc (kind, callback);

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all (sd, 1, cgen_cpu_max_extra_bytes ()) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

#if 0 /* FIXME: pc is in mach-specific struct */
  /* FIXME: watchpoints code shouldn't need this */
  {
    SIM_CPU *current_cpu = STATE_CPU (sd, 0);
    STATE_WATCHPOINTS (sd)->pc = &(PC);
    STATE_WATCHPOINTS (sd)->sizeof_pc = sizeof (PC);
  }
#endif

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

#if 0 /* FIXME: 'twould be nice if we could do this */
  /* These options override any module options.
     Obviously ambiguity should be avoided, however the caller may wish to
     augment the meaning of an option.  */
  if (extra_options != NULL)
    sim_add_option_table (sd, extra_options);
#endif

  /* getopt will print the error message so we just have to exit if this fails.
     FIXME: Hmmm...  in the case of gdb we need getopt to call
     print_filtered.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Allocate a handler for the control registers and other devices
     if no memory for that range has been allocated by the user.
     All are allocated in one chunk to keep things from being
     unnecessarily complicated.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, I960_DEVICE_ADDR, 1) == 0)
    sim_core_attach (sd, NULL,
		     0 /*level*/,
		     access_read_write,
		     0 /*space ???*/,
		     I960_DEVICE_ADDR, I960_DEVICE_LEN /*nr_bytes*/,
		     0 /*modulo*/,
		     &i960_devices,
		     NULL /*buffer*/);

  /* Allocate core managed memory if none specified by user.
     Use address 4 here in case the user wanted address 0 unmapped.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, 4, 1) == 0)
    /* ??? wilson */
    sim_do_commandf (sd, "memory region 0x%lx,0x%lx", I960_DEFAULT_MEM_START,
		     I960_DEFAULT_MEM_SIZE);

  /* check for/establish the reference program image */
  if (sim_analyze_program (sd,
			   (STATE_PROG_ARGV (sd) != NULL
			    ? *STATE_PROG_ARGV (sd)
			    : NULL),
			   abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Establish any remaining configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Open a copy of the cpu descriptor table.  */
  {
    CGEN_CPU_DESC cd = i960_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
					     CGEN_ENDIAN_LITTLE);
    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	SIM_CPU *cpu = STATE_CPU (sd, i);
	CPU_CPU_DESC (cpu) = cd;
	CPU_DISASSEMBLER (cpu) = i960_disassemble_insn;
      }
  }

  /* Initialize various cgen things not done by common framework.
     Must be done after i960_cgen_cpu_open.  */
  cgen_init (sd);

  /* Store in a global so things like i960_dump_regs can be invoked
     from the gdb command line.  */
  current_state = sd;

  return sd;
}

void
sim_close (sd, quitting)
     SIM_DESC sd;
     int quitting;
{
  i960_cgen_cpu_close (CPU_CPU_DESC (STATE_CPU (sd, 0)));
  sim_module_uninstall (sd);
}

SIM_RC
sim_create_inferior (sd, abfd, argv, envp)
     SIM_DESC sd;
     struct bfd *abfd;
     char **argv;
     char **envp;
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);
  SIM_ADDR addr;

  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;
  sim_pc_set (current_cpu, addr);

#if 0
  STATE_ARGV (sd) = sim_copy_argv (argv);
  STATE_ENVP (sd) = sim_copy_argv (envp);
#endif

  return SIM_RC_OK;
}

#if 0
int
sim_stop (SIM_DESC sd)
{
  switch (STATE_ARCHITECTURE (sd)->mach)
    {
    case bfd_mach_i960_ka_sa :
      return i960base_engine_stop (sd, NULL, NULL_CIA, sim_stopped, SIM_SIGINT);
    default :
      abort ();
    }
}

/* This isn't part of the official interface.
   This is just a good place to put this for now.  */

void
sim_sync_stop (SIM_DESC sd, SIM_CPU *cpu, PCADDR pc, enum sim_stop reason, int sigrc)
{
  switch (STATE_ARCHITECTURE (sd)->mach)
    {
    case bfd_mach_i960_ka_sa :
      (void) i960base_engine_stop (sd, cpu, pc, reason, sigrc);
      break;
    default :
      abort ();
    }
}

void
sim_resume (sd, step, siggnal)
     SIM_DESC sd;
     int step, siggnal;
{
  sim_module_resume (sd);

  switch (STATE_ARCHITECTURE (sd)->mach)
    {
    case bfd_mach_i960_ka_sa :
      i960base_engine_run (sd, step, siggnal);
      break;
    default :
      abort ();
    }

  sim_module_suspend (sd);
}
#endif

/* The contents of BUF are in target byte order.  */

int
sim_fetch_register (sd, rn, buf, length)
     SIM_DESC sd;
     int rn;
     unsigned char *buf;
     int length;
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);

  return (* CPU_REG_FETCH (cpu)) (cpu, rn, buf, length);
}
 
/* The contents of BUF are in target byte order.  */

int
sim_store_register (sd, rn, buf, length)
     SIM_DESC sd;
     int rn;
     unsigned char *buf;
     int length;
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);

  return (* CPU_REG_STORE (cpu)) (cpu, rn, buf, length);
}

void
sim_do_command (sd, cmd)
     SIM_DESC sd;
     char *cmd;
{ 
  if (sim_args_command (sd, cmd) != SIM_RC_OK)
    sim_io_eprintf (sd, "Unknown command `%s'\n", cmd);
}

/* Disassemble an instruction.  */

static void
i960_disassemble_insn (SIM_CPU *cpu, const CGEN_INSN *insn,
		       const ARGBUF *abuf, IADDR pc, char *buf)
{
  struct disassemble_info disasm_info;
  SFILE sfile;
  SIM_DESC sd = CPU_STATE (cpu);
  int insn_length = CGEN_INSN_BITSIZE (insn) / 8;

  sfile.buffer = sfile.current = buf;
  INIT_DISASSEMBLE_INFO (disasm_info, (FILE *) &sfile,
			 (fprintf_ftype) sim_disasm_sprintf);
  disasm_info.endian =
    (bfd_big_endian (STATE_PROG_BFD (sd)) ? BFD_ENDIAN_BIG
     : bfd_little_endian (STATE_PROG_BFD (sd)) ? BFD_ENDIAN_LITTLE
     : BFD_ENDIAN_UNKNOWN);
  disasm_info.read_memory_func = sim_disasm_read_memory;
  disasm_info.memory_error_func = sim_disasm_perror_memory;
  disasm_info.application_data = (PTR) cpu;

  print_insn_i960 (pc, &disasm_info);
}
