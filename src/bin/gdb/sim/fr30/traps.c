// OBSOLETE /* fr30 exception, interrupt, and trap (EIT) support
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
// OBSOLETE #include "targ-vals.h"
// OBSOLETE #include "cgen-engine.h"
// OBSOLETE 
// OBSOLETE /* The semantic code invokes this for invalid (unrecognized) instructions.  */
// OBSOLETE 
// OBSOLETE SEM_PC
// OBSOLETE sim_engine_invalid_insn (SIM_CPU *current_cpu, IADDR cia, SEM_PC vpc)
// OBSOLETE {
// OBSOLETE   SIM_DESC sd = CPU_STATE (current_cpu);
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE   if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
// OBSOLETE     {
// OBSOLETE       h_bsm_set (current_cpu, h_sm_get (current_cpu));
// OBSOLETE       h_bie_set (current_cpu, h_ie_get (current_cpu));
// OBSOLETE       h_bcond_set (current_cpu, h_cond_get (current_cpu));
// OBSOLETE       /* sm not changed */
// OBSOLETE       h_ie_set (current_cpu, 0);
// OBSOLETE       h_cond_set (current_cpu, 0);
// OBSOLETE 
// OBSOLETE       h_bpc_set (current_cpu, cia);
// OBSOLETE 
// OBSOLETE       sim_engine_restart (CPU_STATE (current_cpu), current_cpu, NULL,
// OBSOLETE 			  EIT_RSVD_INSN_ADDR);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE #endif
// OBSOLETE     sim_engine_halt (sd, current_cpu, NULL, cia, sim_stopped, SIM_SIGILL);
// OBSOLETE   return vpc;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Process an address exception.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30_core_signal (SIM_DESC sd, SIM_CPU *current_cpu, sim_cia cia,
// OBSOLETE 		  unsigned int map, int nr_bytes, address_word addr,
// OBSOLETE 		  transfer_type transfer, sim_core_signals sig)
// OBSOLETE {
// OBSOLETE #if 0
// OBSOLETE   if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
// OBSOLETE     {
// OBSOLETE       h_bsm_set (current_cpu, h_sm_get (current_cpu));
// OBSOLETE       h_bie_set (current_cpu, h_ie_get (current_cpu));
// OBSOLETE       h_bcond_set (current_cpu, h_cond_get (current_cpu));
// OBSOLETE       /* sm not changed */
// OBSOLETE       h_ie_set (current_cpu, 0);
// OBSOLETE       h_cond_set (current_cpu, 0);
// OBSOLETE 
// OBSOLETE       h_bpc_set (current_cpu, cia);
// OBSOLETE 
// OBSOLETE       sim_engine_restart (CPU_STATE (current_cpu), current_cpu, NULL,
// OBSOLETE 			  EIT_ADDR_EXCP_ADDR);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE #endif
// OBSOLETE     sim_core_signal (sd, current_cpu, cia, map, nr_bytes, addr,
// OBSOLETE 		     transfer, sig);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Read/write functions for system call interface.  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE syscall_read_mem (host_callback *cb, struct cb_syscall *sc,
// OBSOLETE 		  unsigned long taddr, char *buf, int bytes)
// OBSOLETE {
// OBSOLETE   SIM_DESC sd = (SIM_DESC) sc->p1;
// OBSOLETE   SIM_CPU *cpu = (SIM_CPU *) sc->p2;
// OBSOLETE 
// OBSOLETE   return sim_core_read_buffer (sd, cpu, read_map, buf, taddr, bytes);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE syscall_write_mem (host_callback *cb, struct cb_syscall *sc,
// OBSOLETE 		   unsigned long taddr, const char *buf, int bytes)
// OBSOLETE {
// OBSOLETE   SIM_DESC sd = (SIM_DESC) sc->p1;
// OBSOLETE   SIM_CPU *cpu = (SIM_CPU *) sc->p2;
// OBSOLETE 
// OBSOLETE   return sim_core_write_buffer (sd, cpu, write_map, buf, taddr, bytes);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Subroutine of fr30_int to save the PS and PC and setup for INT and INTE.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE setup_int (SIM_CPU *current_cpu, PCADDR pc)
// OBSOLETE {
// OBSOLETE   USI ssp = fr30bf_h_dr_get (current_cpu, H_DR_SSP);
// OBSOLETE   USI ps = fr30bf_h_ps_get (current_cpu);
// OBSOLETE 
// OBSOLETE   ssp -= 4;
// OBSOLETE   SETMEMSI (current_cpu, pc, ssp, ps);
// OBSOLETE   ssp -= 4;
// OBSOLETE   SETMEMSI (current_cpu, pc, ssp, pc + 2);
// OBSOLETE   fr30bf_h_dr_set (current_cpu, H_DR_SSP, ssp);
// OBSOLETE   fr30bf_h_sbit_set (current_cpu, 0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Trap support.
// OBSOLETE    The result is the pc address to continue at.
// OBSOLETE    Preprocessing like saving the various registers has already been done.  */
// OBSOLETE 
// OBSOLETE USI
// OBSOLETE fr30_int (SIM_CPU *current_cpu, PCADDR pc, int num)
// OBSOLETE {
// OBSOLETE   SIM_DESC sd = CPU_STATE (current_cpu);
// OBSOLETE   host_callback *cb = STATE_CALLBACK (sd);
// OBSOLETE 
// OBSOLETE #ifdef SIM_HAVE_BREAKPOINTS
// OBSOLETE   /* Check for breakpoints "owned" by the simulator first, regardless
// OBSOLETE      of --environment.  */
// OBSOLETE   if (num == TRAP_BREAKPOINT)
// OBSOLETE     {
// OBSOLETE       /* First try sim-break.c.  If it's a breakpoint the simulator "owns"
// OBSOLETE 	 it doesn't return.  Otherwise it returns and let's us try.  */
// OBSOLETE       sim_handle_breakpoint (sd, current_cpu, pc);
// OBSOLETE       /* Fall through.  */
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
// OBSOLETE     {
// OBSOLETE       /* The new pc is the trap vector entry.
// OBSOLETE 	 We assume there's a branch there to some handler.  */
// OBSOLETE       USI new_pc;
// OBSOLETE       setup_int (current_cpu, pc);
// OBSOLETE       fr30bf_h_ibit_set (current_cpu, 0);
// OBSOLETE       new_pc = GETMEMSI (current_cpu, pc,
// OBSOLETE 			 fr30bf_h_dr_get (current_cpu, H_DR_TBR)
// OBSOLETE 			 + 1024 - ((num + 1) * 4));
// OBSOLETE       return new_pc;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   switch (num)
// OBSOLETE     {
// OBSOLETE     case TRAP_SYSCALL :
// OBSOLETE       {
// OBSOLETE 	/* TODO: find out what the ABI for this is */
// OBSOLETE 	CB_SYSCALL s;
// OBSOLETE 
// OBSOLETE 	CB_SYSCALL_INIT (&s);
// OBSOLETE 	s.func = fr30bf_h_gr_get (current_cpu, 0);
// OBSOLETE 	s.arg1 = fr30bf_h_gr_get (current_cpu, 4);
// OBSOLETE 	s.arg2 = fr30bf_h_gr_get (current_cpu, 5);
// OBSOLETE 	s.arg3 = fr30bf_h_gr_get (current_cpu, 6);
// OBSOLETE 
// OBSOLETE 	if (s.func == TARGET_SYS_exit)
// OBSOLETE 	  {
// OBSOLETE 	    sim_engine_halt (sd, current_cpu, NULL, pc, sim_exited, s.arg1);
// OBSOLETE 	  }
// OBSOLETE 
// OBSOLETE 	s.p1 = (PTR) sd;
// OBSOLETE 	s.p2 = (PTR) current_cpu;
// OBSOLETE 	s.read_mem = syscall_read_mem;
// OBSOLETE 	s.write_mem = syscall_write_mem;
// OBSOLETE 	cb_syscall (cb, &s);
// OBSOLETE 	fr30bf_h_gr_set (current_cpu, 2, s.errcode); /* TODO: check this one */
// OBSOLETE 	fr30bf_h_gr_set (current_cpu, 4, s.result);
// OBSOLETE 	fr30bf_h_gr_set (current_cpu, 1, s.result2); /* TODO: check this one */
// OBSOLETE 	break;
// OBSOLETE       }
// OBSOLETE 
// OBSOLETE     case TRAP_BREAKPOINT:
// OBSOLETE       sim_engine_halt (sd, current_cpu, NULL, pc,
// OBSOLETE 		       sim_stopped, SIM_SIGTRAP);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     default :
// OBSOLETE       {
// OBSOLETE 	USI new_pc;
// OBSOLETE 	setup_int (current_cpu, pc);
// OBSOLETE 	fr30bf_h_ibit_set (current_cpu, 0);
// OBSOLETE 	new_pc = GETMEMSI (current_cpu, pc,
// OBSOLETE 			   fr30bf_h_dr_get (current_cpu, H_DR_TBR)
// OBSOLETE 			   + 1024 - ((num + 1) * 4));
// OBSOLETE 	return new_pc;
// OBSOLETE       }
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Fake an "reti" insn.
// OBSOLETE      Since we didn't push anything to stack, all we need to do is
// OBSOLETE      update pc.  */
// OBSOLETE   return pc + 2;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE USI
// OBSOLETE fr30_inte (SIM_CPU *current_cpu, PCADDR pc, int num)
// OBSOLETE {
// OBSOLETE   /* The new pc is the trap #9 vector entry.
// OBSOLETE      We assume there's a branch there to some handler.  */
// OBSOLETE   USI new_pc;
// OBSOLETE   setup_int (current_cpu, pc);
// OBSOLETE   fr30bf_h_ilm_set (current_cpu, 4);
// OBSOLETE   new_pc = GETMEMSI (current_cpu, pc,
// OBSOLETE 		     fr30bf_h_dr_get (current_cpu, H_DR_TBR)
// OBSOLETE 		     + 1024 - ((9 + 1) * 4));
// OBSOLETE   return new_pc;
// OBSOLETE }
