/* OBSOLETE /*  This file is part of the program psim. */
/* OBSOLETE  */
/* OBSOLETE     Copyright (C) 1994-1996, Andrew Cagney <cagney@highland.com.au> */
/* OBSOLETE     Copyright (C) 1997, Free Software Foundation */
/* OBSOLETE  */
/* OBSOLETE     This program is free software; you can redistribute it and/or modify */
/* OBSOLETE     it under the terms of the GNU General Public License as published by */
/* OBSOLETE     the Free Software Foundation; either version 2 of the License, or */
/* OBSOLETE     (at your option) any later version. */
/* OBSOLETE  */
/* OBSOLETE     This program is distributed in the hope that it will be useful, */
/* OBSOLETE     but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE     GNU General Public License for more details. */
/* OBSOLETE   */
/* OBSOLETE     You should have received a copy of the GNU General Public License */
/* OBSOLETE     along with this program; if not, write to the Free Software */
/* OBSOLETE     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */
/* OBSOLETE   */
/* OBSOLETE     */ */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE #include <stdarg.h> */
/* OBSOLETE #include <ctype.h> */
/* OBSOLETE  */
/* OBSOLETE #include "sim-main.h" */
/* OBSOLETE #include "sim-options.h" */
/* OBSOLETE  */
/* OBSOLETE #include "bfd.h" */
/* OBSOLETE #include "sim-utils.h" */
/* OBSOLETE  */
/* OBSOLETE #ifdef HAVE_STDLIB_H */
/* OBSOLETE #include <stdlib.h> */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE static unsigned long extmem_size = 1024*1024*8;	/* 8 meg is the maximum listed in the arch. manual */ */
/* OBSOLETE  */
/* OBSOLETE static const char * get_insn_name (sim_cpu *, int); */
/* OBSOLETE  */
/* OBSOLETE #define SIM_ADDR unsigned */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE #define OPTION_TRACE_CALL	200 */
/* OBSOLETE #define OPTION_TRACE_TRAPDUMP	201 */
/* OBSOLETE #define OPTION_EXTMEM_SIZE	202 */
/* OBSOLETE  */
/* OBSOLETE static SIM_RC */
/* OBSOLETE d30v_option_handler (SIM_DESC sd, */
/* OBSOLETE 		     sim_cpu *cpu, */
/* OBSOLETE 		     int opt, */
/* OBSOLETE 		     char *arg, */
/* OBSOLETE 		     int command_p) */
/* OBSOLETE { */
/* OBSOLETE   char *suffix; */
/* OBSOLETE  */
/* OBSOLETE   switch (opt) */
/* OBSOLETE     { */
/* OBSOLETE     default: */
/* OBSOLETE       break; */
/* OBSOLETE  */
/* OBSOLETE     case OPTION_TRACE_CALL: */
/* OBSOLETE       if (arg == NULL || strcmp (arg, "yes") == 0 || strcmp (arg, "on") == 0) */
/* OBSOLETE 	TRACE_CALL_P = 1; */
/* OBSOLETE       else if (strcmp (arg, "no") == 0 || strcmp (arg, "off") == 0) */
/* OBSOLETE 	TRACE_CALL_P = 0; */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE 	  sim_io_eprintf (sd, "Unreconized --trace-call option `%s'\n", arg); */
/* OBSOLETE 	  return SIM_RC_FAIL; */
/* OBSOLETE 	} */
/* OBSOLETE       return SIM_RC_OK; */
/* OBSOLETE  */
/* OBSOLETE     case OPTION_TRACE_TRAPDUMP: */
/* OBSOLETE       if (arg == NULL || strcmp (arg, "yes") == 0 || strcmp (arg, "on") == 0) */
/* OBSOLETE 	TRACE_TRAP_P = 1; */
/* OBSOLETE       else if (strcmp (arg, "no") == 0 || strcmp (arg, "off") == 0) */
/* OBSOLETE 	TRACE_TRAP_P = 0; */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE 	  sim_io_eprintf (sd, "Unreconized --trace-call option `%s'\n", arg); */
/* OBSOLETE 	  return SIM_RC_FAIL; */
/* OBSOLETE 	} */
/* OBSOLETE       return SIM_RC_OK; */
/* OBSOLETE  */
/* OBSOLETE     case OPTION_EXTMEM_SIZE: */
/* OBSOLETE       if (arg == NULL || !isdigit (*arg)) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  sim_io_eprintf (sd, "Invalid memory size `%s'", arg); */
/* OBSOLETE 	  return SIM_RC_FAIL; */
/* OBSOLETE 	} */
/* OBSOLETE  */
/* OBSOLETE       suffix = arg; */
/* OBSOLETE       extmem_size = strtol (arg, &suffix, 0); */
/* OBSOLETE       if (*suffix == 'm' || *suffix == 'M') */
/* OBSOLETE 	extmem_size <<= 20; */
/* OBSOLETE       else if (*suffix == 'k' || *suffix == 'K') */
/* OBSOLETE 	extmem_size <<= 10; */
/* OBSOLETE       sim_do_commandf (sd, "memory delete 0x80000000"); */
/* OBSOLETE       sim_do_commandf (sd, "memory region 0x80000000,0x%lx", extmem_size); */
/* OBSOLETE  */
/* OBSOLETE       return SIM_RC_OK; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   sim_io_eprintf (sd, "Unknown option (%d)\n", opt); */
/* OBSOLETE   return SIM_RC_FAIL; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static const OPTION d30v_options[] = */
/* OBSOLETE { */
/* OBSOLETE   { {"trace-call", optional_argument, NULL, OPTION_TRACE_CALL}, */
/* OBSOLETE       '\0', "on|off", "Enable tracing of calls and returns, checking saved registers", */
/* OBSOLETE       d30v_option_handler }, */
/* OBSOLETE   { {"trace-trapdump", optional_argument, NULL, OPTION_TRACE_TRAPDUMP}, */
/* OBSOLETE       '\0', "on|off", */
/* OBSOLETE #if TRAPDUMP */
/* OBSOLETE     "Traps 0..30 dump out all of the registers (defaults on)", */
/* OBSOLETE #else */
/* OBSOLETE     "Traps 0..30 dump out all of the registers", */
/* OBSOLETE #endif */
/* OBSOLETE       d30v_option_handler }, */
/* OBSOLETE   { {"extmem-size", required_argument, NULL, OPTION_EXTMEM_SIZE}, */
/* OBSOLETE     '\0', "size", "Change size of external memory, default 8 meg", */
/* OBSOLETE     d30v_option_handler }, */
/* OBSOLETE   { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL } */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE /* Return name of an insn, used by insn profiling.  */ */
/* OBSOLETE  */
/* OBSOLETE static const char * */
/* OBSOLETE get_insn_name (sim_cpu *cpu, int i) */
/* OBSOLETE { */
/* OBSOLETE   return itable[i].name; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Structures used by the simulator, for gdb just have static structures */ */
/* OBSOLETE  */
/* OBSOLETE SIM_DESC */
/* OBSOLETE sim_open (SIM_OPEN_KIND kind, */
/* OBSOLETE 	  host_callback *callback, */
/* OBSOLETE 	  struct _bfd *abfd, */
/* OBSOLETE 	  char **argv) */
/* OBSOLETE { */
/* OBSOLETE   SIM_DESC sd = sim_state_alloc (kind, callback); */
/* OBSOLETE  */
/* OBSOLETE   /* FIXME: watchpoints code shouldn't need this */ */
/* OBSOLETE   STATE_WATCHPOINTS (sd)->pc = &(PC); */
/* OBSOLETE   STATE_WATCHPOINTS (sd)->sizeof_pc = sizeof (PC); */
/* OBSOLETE   STATE_WATCHPOINTS (sd)->interrupt_handler = d30v_interrupt_event; */
/* OBSOLETE  */
/* OBSOLETE   /* Initialize the mechanism for doing insn profiling.  */ */
/* OBSOLETE   CPU_INSN_NAME (STATE_CPU (sd, 0)) = get_insn_name; */
/* OBSOLETE   CPU_MAX_INSNS (STATE_CPU (sd, 0)) = nr_itable_entries; */
/* OBSOLETE  */
/* OBSOLETE #ifdef TRAPDUMP */
/* OBSOLETE   TRACE_TRAP_P = TRAPDUMP; */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE   if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK) */
/* OBSOLETE     return 0; */
/* OBSOLETE   sim_add_option_table (sd, NULL, d30v_options); */
/* OBSOLETE  */
/* OBSOLETE   /* Memory and EEPROM */ */
/* OBSOLETE   /* internal instruction RAM - fixed */ */
/* OBSOLETE   sim_do_commandf (sd, "memory region 0,0x10000"); */
/* OBSOLETE   /* internal data RAM - fixed */ */
/* OBSOLETE   sim_do_commandf (sd, "memory region 0x20000000,0x8000"); */
/* OBSOLETE   /* control register dummy area */ */
/* OBSOLETE   sim_do_commandf (sd, "memory region 0x40000000,0x10000"); */
/* OBSOLETE   /* external RAM */ */
/* OBSOLETE   sim_do_commandf (sd, "memory region 0x80000000,0x%lx", extmem_size); */
/* OBSOLETE   /* EIT RAM */ */
/* OBSOLETE   sim_do_commandf (sd, "memory region 0xfffff000,0x1000"); */
/* OBSOLETE  */
/* OBSOLETE   /* getopt will print the error message so we just have to exit if this fails. */
/* OBSOLETE      FIXME: Hmmm...  in the case of gdb we need getopt to call */
/* OBSOLETE      print_filtered.  */ */
/* OBSOLETE   if (sim_parse_args (sd, argv) != SIM_RC_OK) */
/* OBSOLETE     { */
/* OBSOLETE       /* Uninstall the modules to avoid memory leaks, */
/* OBSOLETE 	 file descriptor leaks, etc.  */ */
/* OBSOLETE       sim_module_uninstall (sd); */
/* OBSOLETE       return 0; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* check for/establish the a reference program image */ */
/* OBSOLETE   if (sim_analyze_program (sd, */
/* OBSOLETE 			   (STATE_PROG_ARGV (sd) != NULL */
/* OBSOLETE 			    ? *STATE_PROG_ARGV (sd) */
/* OBSOLETE 			    : NULL), */
/* OBSOLETE 			   abfd) != SIM_RC_OK) */
/* OBSOLETE     { */
/* OBSOLETE       sim_module_uninstall (sd); */
/* OBSOLETE       return 0; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* establish any remaining configuration options */ */
/* OBSOLETE   if (sim_config (sd) != SIM_RC_OK) */
/* OBSOLETE     { */
/* OBSOLETE       sim_module_uninstall (sd); */
/* OBSOLETE       return 0; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   if (sim_post_argv_init (sd) != SIM_RC_OK) */
/* OBSOLETE     { */
/* OBSOLETE       /* Uninstall the modules to avoid memory leaks, */
/* OBSOLETE 	 file descriptor leaks, etc.  */ */
/* OBSOLETE       sim_module_uninstall (sd); */
/* OBSOLETE       return 0; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   return sd; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE sim_close (SIM_DESC sd, int quitting) */
/* OBSOLETE { */
/* OBSOLETE   /* Uninstall the modules to avoid memory leaks, */
/* OBSOLETE      file descriptor leaks, etc.  */ */
/* OBSOLETE   sim_module_uninstall (sd); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE SIM_RC */
/* OBSOLETE sim_create_inferior (SIM_DESC sd, */
/* OBSOLETE 		     struct _bfd *abfd, */
/* OBSOLETE 		     char **argv, */
/* OBSOLETE 		     char **envp) */
/* OBSOLETE { */
/* OBSOLETE   /* clear all registers */ */
/* OBSOLETE   memset (&STATE_CPU (sd, 0)->regs, 0, sizeof (STATE_CPU (sd, 0)->regs)); */
/* OBSOLETE   EIT_VB = EIT_VB_DEFAULT; */
/* OBSOLETE   STATE_CPU (sd, 0)->unit = any_unit; */
/* OBSOLETE   sim_module_init (sd); */
/* OBSOLETE   if (abfd != NULL) */
/* OBSOLETE     PC = bfd_get_start_address (abfd); */
/* OBSOLETE   else */
/* OBSOLETE     PC = 0xfffff000; /* reset value */ */
/* OBSOLETE   return SIM_RC_OK; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE sim_do_command (SIM_DESC sd, char *cmd) */
/* OBSOLETE { */
/* OBSOLETE   if (sim_args_command (sd, cmd) != SIM_RC_OK) */
/* OBSOLETE     sim_io_printf (sd, "Unknown command `%s'\n", cmd); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* The following register definitions were ripped off from */
/* OBSOLETE    gdb/config/tm-d30v.h.  If any of those defs changes, this table needs to */
/* OBSOLETE    be updated.  */ */
/* OBSOLETE  */
/* OBSOLETE #define NUM_REGS 86 */
/* OBSOLETE  */
/* OBSOLETE #define R0_REGNUM	0 */
/* OBSOLETE #define FP_REGNUM	11 */
/* OBSOLETE #define LR_REGNUM 	62 */
/* OBSOLETE #define SP_REGNUM 	63 */
/* OBSOLETE #define SPI_REGNUM	64	/* Interrupt stack pointer */ */
/* OBSOLETE #define SPU_REGNUM	65	/* User stack pointer */ */
/* OBSOLETE #define CREGS_START	66 */
/* OBSOLETE  */
/* OBSOLETE #define PSW_REGNUM 	(CREGS_START + 0) /* psw, bpsw, or dpsw??? */ */
/* OBSOLETE #define    PSW_SM 0x80000000	/* Stack mode: 0 == interrupt (SPI), */
/* OBSOLETE 					       1 == user (SPU) */ */
/* OBSOLETE #define BPSW_REGNUM	(CREGS_START + 1) /* Backup PSW (on interrupt) */ */
/* OBSOLETE #define PC_REGNUM 	(CREGS_START + 2) /* pc, bpc, or dpc??? */ */
/* OBSOLETE #define BPC_REGNUM 	(CREGS_START + 3) /* Backup PC (on interrupt) */ */
/* OBSOLETE #define DPSW_REGNUM	(CREGS_START + 4) /* Backup PSW (on debug trap) */ */
/* OBSOLETE #define DPC_REGNUM 	(CREGS_START + 5) /* Backup PC (on debug trap) */ */
/* OBSOLETE #define RPT_C_REGNUM	(CREGS_START + 7) /* Loop count */ */
/* OBSOLETE #define RPT_S_REGNUM	(CREGS_START + 8) /* Loop start address*/ */
/* OBSOLETE #define RPT_E_REGNUM	(CREGS_START + 9) /* Loop end address */ */
/* OBSOLETE #define MOD_S_REGNUM	(CREGS_START + 10) */
/* OBSOLETE #define MOD_E_REGNUM	(CREGS_START + 11) */
/* OBSOLETE #define IBA_REGNUM	(CREGS_START + 14) /* Instruction break address */ */
/* OBSOLETE #define EIT_VB_REGNUM	(CREGS_START + 15) /* Vector base address */ */
/* OBSOLETE #define INT_S_REGNUM	(CREGS_START + 16) /* Interrupt status */ */
/* OBSOLETE #define INT_M_REGNUM	(CREGS_START + 17) /* Interrupt mask */ */
/* OBSOLETE #define A0_REGNUM 	84 */
/* OBSOLETE #define A1_REGNUM 	85 */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE sim_fetch_register (sd, regno, buf, length) */
/* OBSOLETE      SIM_DESC sd; */
/* OBSOLETE      int regno; */
/* OBSOLETE      unsigned char *buf; */
/* OBSOLETE      int length; */
/* OBSOLETE { */
/* OBSOLETE   if (regno < A0_REGNUM) */
/* OBSOLETE     { */
/* OBSOLETE       unsigned32 reg; */
/* OBSOLETE  */
/* OBSOLETE       if (regno <= R0_REGNUM + 63) */
/* OBSOLETE 	reg = sd->cpu[0].regs.general_purpose[regno]; */
/* OBSOLETE       else if (regno <= SPU_REGNUM) */
/* OBSOLETE 	reg = sd->cpu[0].regs.sp[regno - SPI_REGNUM]; */
/* OBSOLETE       else */
/* OBSOLETE 	reg = sd->cpu[0].regs.control[regno - CREGS_START]; */
/* OBSOLETE  */
/* OBSOLETE       buf[0] = reg >> 24; */
/* OBSOLETE       buf[1] = reg >> 16; */
/* OBSOLETE       buf[2] = reg >> 8; */
/* OBSOLETE       buf[3] = reg; */
/* OBSOLETE     } */
/* OBSOLETE   else if (regno < NUM_REGS) */
/* OBSOLETE     { */
/* OBSOLETE       unsigned32 reg; */
/* OBSOLETE  */
/* OBSOLETE       reg = sd->cpu[0].regs.accumulator[regno - A0_REGNUM] >> 32; */
/* OBSOLETE  */
/* OBSOLETE       buf[0] = reg >> 24; */
/* OBSOLETE       buf[1] = reg >> 16; */
/* OBSOLETE       buf[2] = reg >> 8; */
/* OBSOLETE       buf[3] = reg; */
/* OBSOLETE  */
/* OBSOLETE       reg = sd->cpu[0].regs.accumulator[regno - A0_REGNUM]; */
/* OBSOLETE  */
/* OBSOLETE       buf[4] = reg >> 24; */
/* OBSOLETE       buf[5] = reg >> 16; */
/* OBSOLETE       buf[6] = reg >> 8; */
/* OBSOLETE       buf[7] = reg; */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     abort (); */
/* OBSOLETE   return -1; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE sim_store_register (sd, regno, buf, length) */
/* OBSOLETE      SIM_DESC sd; */
/* OBSOLETE      int regno; */
/* OBSOLETE      unsigned char *buf; */
/* OBSOLETE      int length; */
/* OBSOLETE { */
/* OBSOLETE   if (regno < A0_REGNUM) */
/* OBSOLETE     { */
/* OBSOLETE       unsigned32 reg; */
/* OBSOLETE  */
/* OBSOLETE       reg = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]; */
/* OBSOLETE  */
/* OBSOLETE       if (regno <= R0_REGNUM + 63) */
/* OBSOLETE 	sd->cpu[0].regs.general_purpose[regno] = reg; */
/* OBSOLETE       else if (regno <= SPU_REGNUM) */
/* OBSOLETE 	sd->cpu[0].regs.sp[regno - SPI_REGNUM] = reg; */
/* OBSOLETE       else */
/* OBSOLETE 	sd->cpu[0].regs.control[regno - CREGS_START] = reg; */
/* OBSOLETE     } */
/* OBSOLETE   else if (regno < NUM_REGS) */
/* OBSOLETE     { */
/* OBSOLETE       unsigned32 reg; */
/* OBSOLETE  */
/* OBSOLETE       reg = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]; */
/* OBSOLETE  */
/* OBSOLETE       sd->cpu[0].regs.accumulator[regno - A0_REGNUM] = (unsigned64)reg << 32; */
/* OBSOLETE  */
/* OBSOLETE       reg = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7]; */
/* OBSOLETE  */
/* OBSOLETE       sd->cpu[0].regs.accumulator[regno - A0_REGNUM] |= reg; */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     abort (); */
/* OBSOLETE   return -1; */
/* OBSOLETE } */
