// OBSOLETE /* Main simulator entry points specific to the FR30.
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
// OBSOLETE #include "sim-main.h"
// OBSOLETE #ifdef HAVE_STDLIB_H
// OBSOLETE #include <stdlib.h>
// OBSOLETE #endif
// OBSOLETE #include "sim-options.h"
// OBSOLETE #include "libiberty.h"
// OBSOLETE #include "bfd.h"
// OBSOLETE 
// OBSOLETE static void free_state (SIM_DESC);
// OBSOLETE static void print_fr30_misc_cpu (SIM_CPU *cpu, int verbose);
// OBSOLETE 
// OBSOLETE /* Records simulator descriptor so utilities like fr30_dump_regs can be
// OBSOLETE    called from gdb.  */
// OBSOLETE SIM_DESC current_state;
// OBSOLETE 
// OBSOLETE /* Cover function of sim_state_free to free the cpu buffers as well.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE free_state (SIM_DESC sd)
// OBSOLETE {
// OBSOLETE   if (STATE_MODULES (sd) != NULL)
// OBSOLETE     sim_module_uninstall (sd);
// OBSOLETE   sim_cpu_free_all (sd);
// OBSOLETE   sim_state_free (sd);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Create an instance of the simulator.  */
// OBSOLETE 
// OBSOLETE SIM_DESC
// OBSOLETE sim_open (kind, callback, abfd, argv)
// OBSOLETE      SIM_OPEN_KIND kind;
// OBSOLETE      host_callback *callback;
// OBSOLETE      struct _bfd *abfd;
// OBSOLETE      char **argv;
// OBSOLETE {
// OBSOLETE   char c;
// OBSOLETE   int i;
// OBSOLETE   SIM_DESC sd = sim_state_alloc (kind, callback);
// OBSOLETE 
// OBSOLETE   /* The cpu data is kept in a separately allocated chunk of memory.  */
// OBSOLETE   if (sim_cpu_alloc_all (sd, 1, cgen_cpu_max_extra_bytes ()) != SIM_RC_OK)
// OBSOLETE     {
// OBSOLETE       free_state (sd);
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE #if 0 /* FIXME: pc is in mach-specific struct */
// OBSOLETE   /* FIXME: watchpoints code shouldn't need this */
// OBSOLETE   {
// OBSOLETE     SIM_CPU *current_cpu = STATE_CPU (sd, 0);
// OBSOLETE     STATE_WATCHPOINTS (sd)->pc = &(PC);
// OBSOLETE     STATE_WATCHPOINTS (sd)->sizeof_pc = sizeof (PC);
// OBSOLETE   }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
// OBSOLETE     {
// OBSOLETE       free_state (sd);
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE #if 0 /* FIXME: 'twould be nice if we could do this */
// OBSOLETE   /* These options override any module options.
// OBSOLETE      Obviously ambiguity should be avoided, however the caller may wish to
// OBSOLETE      augment the meaning of an option.  */
// OBSOLETE   if (extra_options != NULL)
// OBSOLETE     sim_add_option_table (sd, extra_options);
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   /* getopt will print the error message so we just have to exit if this fails.
// OBSOLETE      FIXME: Hmmm...  in the case of gdb we need getopt to call
// OBSOLETE      print_filtered.  */
// OBSOLETE   if (sim_parse_args (sd, argv) != SIM_RC_OK)
// OBSOLETE     {
// OBSOLETE       free_state (sd);
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE   /* Allocate a handler for the control registers and other devices
// OBSOLETE      if no memory for that range has been allocated by the user.
// OBSOLETE      All are allocated in one chunk to keep things from being
// OBSOLETE      unnecessarily complicated.  */
// OBSOLETE   if (sim_core_read_buffer (sd, NULL, read_map, &c, FR30_DEVICE_ADDR, 1) == 0)
// OBSOLETE     sim_core_attach (sd, NULL,
// OBSOLETE 		     0 /*level*/,
// OBSOLETE 		     access_read_write,
// OBSOLETE 		     0 /*space ???*/,
// OBSOLETE 		     FR30_DEVICE_ADDR, FR30_DEVICE_LEN /*nr_bytes*/,
// OBSOLETE 		     0 /*modulo*/,
// OBSOLETE 		     &fr30_devices,
// OBSOLETE 		     NULL /*buffer*/);
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   /* Allocate core managed memory if none specified by user.
// OBSOLETE      Use address 4 here in case the user wanted address 0 unmapped.  */
// OBSOLETE   if (sim_core_read_buffer (sd, NULL, read_map, &c, 4, 1) == 0)
// OBSOLETE     sim_do_commandf (sd, "memory region 0,0x%lx", FR30_DEFAULT_MEM_SIZE);
// OBSOLETE 
// OBSOLETE   /* check for/establish the reference program image */
// OBSOLETE   if (sim_analyze_program (sd,
// OBSOLETE 			   (STATE_PROG_ARGV (sd) != NULL
// OBSOLETE 			    ? *STATE_PROG_ARGV (sd)
// OBSOLETE 			    : NULL),
// OBSOLETE 			   abfd) != SIM_RC_OK)
// OBSOLETE     {
// OBSOLETE       free_state (sd);
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Establish any remaining configuration options.  */
// OBSOLETE   if (sim_config (sd) != SIM_RC_OK)
// OBSOLETE     {
// OBSOLETE       free_state (sd);
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (sim_post_argv_init (sd) != SIM_RC_OK)
// OBSOLETE     {
// OBSOLETE       free_state (sd);
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Open a copy of the cpu descriptor table.  */
// OBSOLETE   {
// OBSOLETE     CGEN_CPU_DESC cd = fr30_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
// OBSOLETE 					     CGEN_ENDIAN_BIG);
// OBSOLETE     for (i = 0; i < MAX_NR_PROCESSORS; ++i)
// OBSOLETE       {
// OBSOLETE 	SIM_CPU *cpu = STATE_CPU (sd, i);
// OBSOLETE 	CPU_CPU_DESC (cpu) = cd;
// OBSOLETE 	CPU_DISASSEMBLER (cpu) = sim_cgen_disassemble_insn;
// OBSOLETE       }
// OBSOLETE     fr30_cgen_init_dis (cd);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   /* Initialize various cgen things not done by common framework.
// OBSOLETE      Must be done after fr30_cgen_cpu_open.  */
// OBSOLETE   cgen_init (sd);
// OBSOLETE 
// OBSOLETE   /* Store in a global so things like sparc32_dump_regs can be invoked
// OBSOLETE      from the gdb command line.  */
// OBSOLETE   current_state = sd;
// OBSOLETE 
// OBSOLETE   return sd;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE sim_close (sd, quitting)
// OBSOLETE      SIM_DESC sd;
// OBSOLETE      int quitting;
// OBSOLETE {
// OBSOLETE   fr30_cgen_cpu_close (CPU_CPU_DESC (STATE_CPU (sd, 0)));
// OBSOLETE   sim_module_uninstall (sd);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE SIM_RC
// OBSOLETE sim_create_inferior (sd, abfd, argv, envp)
// OBSOLETE      SIM_DESC sd;
// OBSOLETE      struct _bfd *abfd;
// OBSOLETE      char **argv;
// OBSOLETE      char **envp;
// OBSOLETE {
// OBSOLETE   SIM_CPU *current_cpu = STATE_CPU (sd, 0);
// OBSOLETE   SIM_ADDR addr;
// OBSOLETE 
// OBSOLETE   if (abfd != NULL)
// OBSOLETE     addr = bfd_get_start_address (abfd);
// OBSOLETE   else
// OBSOLETE     addr = 0;
// OBSOLETE   sim_pc_set (current_cpu, addr);
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE   STATE_ARGV (sd) = sim_copy_argv (argv);
// OBSOLETE   STATE_ENVP (sd) = sim_copy_argv (envp);
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   return SIM_RC_OK;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE sim_do_command (sd, cmd)
// OBSOLETE      SIM_DESC sd;
// OBSOLETE      char *cmd;
// OBSOLETE { 
// OBSOLETE   if (sim_args_command (sd, cmd) != SIM_RC_OK)
// OBSOLETE     sim_io_eprintf (sd, "Unknown command `%s'\n", cmd);
// OBSOLETE }
