/* OBSOLETE /* Mitsubishi Electric Corp. D30V Simulator. */
/* OBSOLETE    Copyright (C) 1997, Free Software Foundation, Inc. */
/* OBSOLETE    Contributed by Cygnus Support. */
/* OBSOLETE  */
/* OBSOLETE This file is part of GDB, the GNU debugger. */
/* OBSOLETE  */
/* OBSOLETE This program is free software; you can redistribute it and/or modify */
/* OBSOLETE it under the terms of the GNU General Public License as published by */
/* OBSOLETE the Free Software Foundation; either version 2, or (at your option) */
/* OBSOLETE any later version. */
/* OBSOLETE  */
/* OBSOLETE This program is distributed in the hope that it will be useful, */
/* OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE You should have received a copy of the GNU General Public License along */
/* OBSOLETE with this program; if not, write to the Free Software Foundation, Inc., */
/* OBSOLETE 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */ */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE #ifndef _CPU_C_ */
/* OBSOLETE #define _CPU_C_ */
/* OBSOLETE  */
/* OBSOLETE #include "sim-main.h" */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE is_wrong_slot (SIM_DESC sd, */
/* OBSOLETE 	       address_word cia, */
/* OBSOLETE 	       itable_index index) */
/* OBSOLETE { */
/* OBSOLETE   switch (STATE_CPU (sd, 0)->unit) */
/* OBSOLETE     { */
/* OBSOLETE     case memory_unit: */
/* OBSOLETE       return !itable[index].option[itable_option_mu]; */
/* OBSOLETE     case integer_unit: */
/* OBSOLETE       return !itable[index].option[itable_option_iu]; */
/* OBSOLETE     case any_unit: */
/* OBSOLETE       return 0; */
/* OBSOLETE     default: */
/* OBSOLETE       sim_engine_abort (sd, STATE_CPU (sd, 0), cia, */
/* OBSOLETE 			"internal error - is_wrong_slot - bad switch"); */
/* OBSOLETE       return -1; */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE is_condition_ok (SIM_DESC sd, */
/* OBSOLETE 		 address_word cia, */
/* OBSOLETE 		 int cond) */
/* OBSOLETE { */
/* OBSOLETE   switch (cond) */
/* OBSOLETE     { */
/* OBSOLETE     case 0x0: */
/* OBSOLETE       return 1; */
/* OBSOLETE     case 0x1: */
/* OBSOLETE       return PSW_VAL(PSW_F0); */
/* OBSOLETE     case 0x2: */
/* OBSOLETE       return !PSW_VAL(PSW_F0); */
/* OBSOLETE     case 0x3: */
/* OBSOLETE       return PSW_VAL(PSW_F1); */
/* OBSOLETE     case 0x4: */
/* OBSOLETE       return !PSW_VAL(PSW_F1); */
/* OBSOLETE     case 0x5: */
/* OBSOLETE       return PSW_VAL(PSW_F0) && PSW_VAL(PSW_F1); */
/* OBSOLETE     case 0x6: */
/* OBSOLETE       return PSW_VAL(PSW_F0) && !PSW_VAL(PSW_F1); */
/* OBSOLETE     case 0x7: */
/* OBSOLETE       sim_engine_abort (sd, STATE_CPU (sd, 0), cia, */
/* OBSOLETE 			"is_condition_ok - bad instruction condition bits"); */
/* OBSOLETE       return 0; */
/* OBSOLETE     default: */
/* OBSOLETE       sim_engine_abort (sd, STATE_CPU (sd, 0), cia, */
/* OBSOLETE 			"internal error - is_condition_ok - bad switch"); */
/* OBSOLETE       return -1; */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* If --trace-call, trace calls, remembering the current state of */
/* OBSOLETE    registers.  */ */
/* OBSOLETE  */
/* OBSOLETE typedef struct _call_stack { */
/* OBSOLETE   struct _call_stack *prev; */
/* OBSOLETE   registers regs; */
/* OBSOLETE } call_stack; */
/* OBSOLETE  */
/* OBSOLETE static call_stack *call_stack_head = (call_stack *)0; */
/* OBSOLETE static int call_depth = 0; */
/* OBSOLETE  */
/* OBSOLETE void call_occurred (SIM_DESC sd, */
/* OBSOLETE 		    sim_cpu *cpu, */
/* OBSOLETE 		    address_word cia, */
/* OBSOLETE 		    address_word nia) */
/* OBSOLETE { */
/* OBSOLETE   call_stack *ptr = ZALLOC (call_stack); */
/* OBSOLETE   ptr->regs = cpu->regs; */
/* OBSOLETE   ptr->prev = call_stack_head; */
/* OBSOLETE   call_stack_head = ptr; */
/* OBSOLETE  */
/* OBSOLETE   trace_one_insn (sd, cpu, nia, 1, "", 0, "call", */
/* OBSOLETE 		  "Depth %3d, Return 0x%.8lx, Args 0x%.8lx 0x%.8lx", */
/* OBSOLETE 		  ++call_depth, (unsigned long)cia+8, (unsigned long)GPR[2], */
/* OBSOLETE 		  (unsigned long)GPR[3]); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* If --trace-call, trace returns, checking if any saved register was changed.  */ */
/* OBSOLETE  */
/* OBSOLETE void return_occurred (SIM_DESC sd, */
/* OBSOLETE 		      sim_cpu *cpu, */
/* OBSOLETE 		      address_word cia, */
/* OBSOLETE 		      address_word nia) */
/* OBSOLETE { */
/* OBSOLETE   char buffer[1024]; */
/* OBSOLETE   char *buf_ptr = buffer; */
/* OBSOLETE   call_stack *ptr = call_stack_head; */
/* OBSOLETE   int regno; */
/* OBSOLETE   char *prefix = ", Registers that differ: "; */
/* OBSOLETE  */
/* OBSOLETE   *buf_ptr = '\0'; */
/* OBSOLETE   for (regno = 34; regno <= 63; regno++) { */
/* OBSOLETE     if (cpu->regs.general_purpose[regno] != ptr->regs.general_purpose[regno]) { */
/* OBSOLETE       sprintf (buf_ptr, "%sr%d", prefix, regno); */
/* OBSOLETE       buf_ptr += strlen (buf_ptr); */
/* OBSOLETE       prefix = " "; */
/* OBSOLETE     } */
/* OBSOLETE   } */
/* OBSOLETE  */
/* OBSOLETE   if (cpu->regs.accumulator[1] != ptr->regs.accumulator[1]) { */
/* OBSOLETE     sprintf (buf_ptr, "%sa1", prefix); */
/* OBSOLETE     buf_ptr += strlen (buf_ptr); */
/* OBSOLETE     prefix = " "; */
/* OBSOLETE   } */
/* OBSOLETE  */
/* OBSOLETE   trace_one_insn (sd, cpu, cia, 1, "", 0, "return", */
/* OBSOLETE 		  "Depth %3d, Return 0x%.8lx, Ret. 0x%.8lx 0x%.8lx%s", */
/* OBSOLETE 		  call_depth--, (unsigned long)nia, (unsigned long)GPR[2], */
/* OBSOLETE 		  (unsigned long)GPR[3], buffer); */
/* OBSOLETE  */
/* OBSOLETE   call_stack_head = ptr->prev; */
/* OBSOLETE   zfree (ptr); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Read/write functions for system call interface.  */ */
/* OBSOLETE int */
/* OBSOLETE d30v_read_mem (host_callback *cb, */
/* OBSOLETE 	       struct cb_syscall *sc, */
/* OBSOLETE 	       unsigned long taddr, */
/* OBSOLETE 	       char *buf, */
/* OBSOLETE 	       int bytes) */
/* OBSOLETE { */
/* OBSOLETE   SIM_DESC sd = (SIM_DESC) sc->p1; */
/* OBSOLETE   sim_cpu *cpu = STATE_CPU (sd, 0); */
/* OBSOLETE  */
/* OBSOLETE   return sim_core_read_buffer (sd, cpu, read_map, buf, taddr, bytes); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE d30v_write_mem (host_callback *cb, */
/* OBSOLETE 		struct cb_syscall *sc, */
/* OBSOLETE 		unsigned long taddr, */
/* OBSOLETE 		const char *buf, */
/* OBSOLETE 		int bytes) */
/* OBSOLETE { */
/* OBSOLETE   SIM_DESC sd = (SIM_DESC) sc->p1; */
/* OBSOLETE   sim_cpu *cpu = STATE_CPU (sd, 0); */
/* OBSOLETE  */
/* OBSOLETE   return sim_core_write_buffer (sd, cpu, write_map, buf, taddr, bytes); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #endif /* _CPU_C_ */ */
