/* OBSOLETE /*  This file is part of the program psim. */
/* OBSOLETE  */
/* OBSOLETE     Copyright (C) 1994-1997, Andrew Cagney <cagney@highland.com.au> */
/* OBSOLETE     Copyright (C) 1996, 1997, Free Software Foundation */
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
/* OBSOLETE #ifndef ENGINE_C */
/* OBSOLETE #define ENGINE_C */
/* OBSOLETE  */
/* OBSOLETE #include "sim-main.h" */
/* OBSOLETE  */
/* OBSOLETE #include <stdio.h> */
/* OBSOLETE #include <ctype.h> */
/* OBSOLETE  */
/* OBSOLETE #ifdef HAVE_STDLIB_H */
/* OBSOLETE #include <stdlib.h> */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE #ifdef HAVE_STRING_H */
/* OBSOLETE #include <string.h> */
/* OBSOLETE #else */
/* OBSOLETE #ifdef HAVE_STRINGS_H */
/* OBSOLETE #include <strings.h> */
/* OBSOLETE #endif */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE do_stack_swap (SIM_DESC sd) */
/* OBSOLETE { */
/* OBSOLETE   sim_cpu *cpu = STATE_CPU (sd, 0); */
/* OBSOLETE   unsigned new_sp = (PSW_VAL(PSW_SM) != 0); */
/* OBSOLETE   if (cpu->regs.current_sp != new_sp) */
/* OBSOLETE     { */
/* OBSOLETE       cpu->regs.sp[cpu->regs.current_sp] = SP; */
/* OBSOLETE       cpu->regs.current_sp = new_sp; */
/* OBSOLETE       SP = cpu->regs.sp[cpu->regs.current_sp]; */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #if WITH_TRACE */
/* OBSOLETE /* Implement ALU tracing of 32-bit registers.  */ */
/* OBSOLETE static void */
/* OBSOLETE trace_alu32 (SIM_DESC sd, */
/* OBSOLETE 	     sim_cpu *cpu, */
/* OBSOLETE 	     address_word cia, */
/* OBSOLETE 	     unsigned32 *ptr) */
/* OBSOLETE { */
/* OBSOLETE   unsigned32 value = *ptr; */
/* OBSOLETE  */
/* OBSOLETE   if (ptr >= &GPR[0] && ptr <= &GPR[NR_GENERAL_PURPOSE_REGISTERS]) */
/* OBSOLETE     trace_one_insn (sd, cpu, cia, 1, "engine.c", __LINE__, "alu", */
/* OBSOLETE 		    "Set register r%-2d = 0x%.8lx (%ld)", */
/* OBSOLETE 		    ptr - &GPR[0], (long)value, (long)value); */
/* OBSOLETE  */
/* OBSOLETE   else if (ptr == &PSW || ptr == &bPSW || ptr == &DPSW) */
/* OBSOLETE     trace_one_insn (sd, cpu, cia, 1, "engine.c", __LINE__, "alu", */
/* OBSOLETE 		    "Set register %s = 0x%.8lx%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", */
/* OBSOLETE 		    (ptr == &PSW) ? "psw" : ((ptr == &bPSW) ? "bpsw" : "dpsw"), */
/* OBSOLETE 		    (long)value, */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_SM)) ? ", sm" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_EA)) ? ", ea" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_DB)) ? ", db" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_DS)) ? ", ds" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_IE)) ? ", ie" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_RP)) ? ", rp" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_MD)) ? ", md" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_F0)) ? ", f0" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_F1)) ? ", f1" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_F2)) ? ", f2" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_F3)) ? ", f3" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_S))  ? ", s"  : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_V))  ? ", v"  : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_VA)) ? ", va" : "", */
/* OBSOLETE 		    (value & (0x80000000 >> PSW_C))  ? ", c"  : ""); */
/* OBSOLETE  */
/* OBSOLETE   else if (ptr >= &CREG[0] && ptr <= &CREG[NR_CONTROL_REGISTERS]) */
/* OBSOLETE     trace_one_insn (sd, cpu, cia, 1, "engine.c", __LINE__, "alu", */
/* OBSOLETE 		    "Set register cr%d = 0x%.8lx (%ld)", */
/* OBSOLETE 		    ptr - &CREG[0], (long)value, (long)value); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Implement ALU tracing of 32-bit registers.  */ */
/* OBSOLETE static void */
/* OBSOLETE trace_alu64 (SIM_DESC sd, */
/* OBSOLETE 	     sim_cpu *cpu, */
/* OBSOLETE 	     address_word cia, */
/* OBSOLETE 	     unsigned64 *ptr) */
/* OBSOLETE { */
/* OBSOLETE   unsigned64 value = *ptr; */
/* OBSOLETE  */
/* OBSOLETE   if (ptr >= &ACC[0] && ptr <= &ACC[NR_ACCUMULATORS]) */
/* OBSOLETE     trace_one_insn (sd, cpu, cia, 1, "engine.c", __LINE__, "alu", */
/* OBSOLETE 		    "Set register a%-2d = 0x%.8lx 0x%.8lx", */
/* OBSOLETE 		    ptr - &ACC[0], */
/* OBSOLETE 		    (unsigned long)(unsigned32)(value >> 32), */
/* OBSOLETE 		    (unsigned long)(unsigned32)value); */
/* OBSOLETE  */
/* OBSOLETE } */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* Process all of the queued up writes in order now */ */
/* OBSOLETE void */
/* OBSOLETE unqueue_writes (SIM_DESC sd, */
/* OBSOLETE 		sim_cpu *cpu, */
/* OBSOLETE 		address_word cia) */
/* OBSOLETE { */
/* OBSOLETE   int i, num; */
/* OBSOLETE   int did_psw = 0; */
/* OBSOLETE   unsigned32 *psw_addr = &PSW; */
/* OBSOLETE  */
/* OBSOLETE   num = WRITE32_NUM; */
/* OBSOLETE   for (i = 0; i < num; i++) */
/* OBSOLETE     { */
/* OBSOLETE       unsigned32 mask = WRITE32_MASK (i); */
/* OBSOLETE       unsigned32 *ptr = WRITE32_PTR (i); */
/* OBSOLETE       unsigned32 value = (*ptr & ~mask) | (WRITE32_VALUE (i) & mask); */
/* OBSOLETE       int j; */
/* OBSOLETE  */
/* OBSOLETE       if (ptr == psw_addr) */
/* OBSOLETE        { */
/* OBSOLETE 	 /* If MU instruction was not a MVTSYS, resolve PSW */
/* OBSOLETE              contention in favour of IU. */ */
/* OBSOLETE 	  if(! STATE_CPU (sd, 0)->mvtsys_left_p) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      /* Detect contention in parallel writes to the same PSW flags. */
/* OBSOLETE 		 The hardware allows the updates from IU to prevail over */
/* OBSOLETE 		 those from MU. */ */
/* OBSOLETE 	       */
/* OBSOLETE 	      unsigned32 flag_bits = */
/* OBSOLETE 		BIT32 (PSW_F0) | BIT32 (PSW_F1) | */
/* OBSOLETE 		BIT32 (PSW_F2) | BIT32 (PSW_F3) | */
/* OBSOLETE 		BIT32 (PSW_S) | BIT32 (PSW_V) | */
/* OBSOLETE 		BIT32 (PSW_VA) | BIT32 (PSW_C); */
/* OBSOLETE 	      unsigned32 my_flag_bits = mask & flag_bits; */
/* OBSOLETE 	       */
/* OBSOLETE 	      for (j = i + 1; j < num; j++) */
/* OBSOLETE 		if (WRITE32_PTR (j) == psw_addr && /* write to PSW */ */
/* OBSOLETE 		    WRITE32_MASK (j) & my_flag_bits)  /* some of the same flags */ */
/* OBSOLETE 		  { */
/* OBSOLETE 		    /* Recompute local mask & value, to suppress this */
/* OBSOLETE 		       earlier write to the same flag bits. */ */
/* OBSOLETE 		     */
/* OBSOLETE 		    unsigned32 new_mask = mask & ~(WRITE32_MASK (j) & my_flag_bits); */
/* OBSOLETE 		     */
/* OBSOLETE 		    /* There is a special case for the VA (accumulated */
/* OBSOLETE 		       overflow) flag, in that it is only included in the */
/* OBSOLETE 		       second instruction's mask if the overflow */
/* OBSOLETE 		       occurred.  Yet the hardware still suppresses the */
/* OBSOLETE 		       first instruction's update to VA.  So we kludge */
/* OBSOLETE 		       this by inferring PSW_V -> PSW_VA for the second */
/* OBSOLETE 		       instruction. */ */
/* OBSOLETE 		     */
/* OBSOLETE 		    if (WRITE32_MASK (j) & BIT32 (PSW_V)) */
/* OBSOLETE 		      { */
/* OBSOLETE 			new_mask &= ~BIT32 (PSW_VA); */
/* OBSOLETE 		      } */
/* OBSOLETE 		     */
/* OBSOLETE 		    value = (*ptr & ~new_mask) | (WRITE32_VALUE (i) & new_mask); */
/* OBSOLETE 		  } */
/* OBSOLETE 	    } */
/* OBSOLETE 	   */
/* OBSOLETE          did_psw = 1; */
/* OBSOLETE        } */
/* OBSOLETE  */
/* OBSOLETE       *ptr = value; */
/* OBSOLETE  */
/* OBSOLETE #if WITH_TRACE */
/* OBSOLETE       if (TRACE_ALU_P (cpu)) */
/* OBSOLETE 	trace_alu32 (sd, cpu, cia, ptr); */
/* OBSOLETE #endif */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   num = WRITE64_NUM; */
/* OBSOLETE   for (i = 0; i < num; i++) */
/* OBSOLETE     { */
/* OBSOLETE       unsigned64 *ptr = WRITE64_PTR (i); */
/* OBSOLETE       *ptr = WRITE64_VALUE (i); */
/* OBSOLETE  */
/* OBSOLETE #if WITH_TRACE */
/* OBSOLETE       if (TRACE_ALU_P (cpu)) */
/* OBSOLETE 	trace_alu64 (sd, cpu, cia, ptr); */
/* OBSOLETE #endif */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   WRITE32_NUM = 0; */
/* OBSOLETE   WRITE64_NUM = 0; */
/* OBSOLETE  */
/* OBSOLETE   if (DID_TRAP == 1) /* ordinary trap */ */
/* OBSOLETE     { */
/* OBSOLETE       bPSW = PSW; */
/* OBSOLETE       PSW &= (BIT32 (PSW_DB) | BIT32 (PSW_SM)); */
/* OBSOLETE       did_psw = 1; */
/* OBSOLETE     } */
/* OBSOLETE   else if (DID_TRAP == 2) /* debug trap */ */
/* OBSOLETE     { */
/* OBSOLETE       DPSW = PSW; */
/* OBSOLETE       PSW &= BIT32 (PSW_DS); */
/* OBSOLETE       PSW |= BIT32 (PSW_DS); */
/* OBSOLETE       did_psw = 1; */
/* OBSOLETE     } */
/* OBSOLETE   DID_TRAP = 0; */
/* OBSOLETE  */
/* OBSOLETE   if (did_psw) */
/* OBSOLETE     do_stack_swap (sd); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* SIMULATE INSTRUCTIONS, various different ways of achieving the same */
/* OBSOLETE    thing */ */
/* OBSOLETE  */
/* OBSOLETE static address_word */
/* OBSOLETE do_long (SIM_DESC sd, */
/* OBSOLETE 	 l_instruction_word instruction, */
/* OBSOLETE 	 address_word cia) */
/* OBSOLETE { */
/* OBSOLETE   address_word nia = l_idecode_issue(sd, */
/* OBSOLETE 				     instruction, */
/* OBSOLETE 				     cia); */
/* OBSOLETE  */
/* OBSOLETE   unqueue_writes (sd, STATE_CPU (sd, 0), cia); */
/* OBSOLETE   return nia; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static address_word */
/* OBSOLETE do_2_short (SIM_DESC sd, */
/* OBSOLETE 	    s_instruction_word insn1, */
/* OBSOLETE 	    s_instruction_word insn2, */
/* OBSOLETE 	    cpu_units unit, */
/* OBSOLETE 	    address_word cia) */
/* OBSOLETE { */
/* OBSOLETE   address_word nia; */
/* OBSOLETE  */
/* OBSOLETE   /* run the first instruction */ */
/* OBSOLETE   STATE_CPU (sd, 0)->unit = unit; */
/* OBSOLETE   STATE_CPU (sd, 0)->left_kills_right_p = 0; */
/* OBSOLETE   STATE_CPU (sd, 0)->mvtsys_left_p = 0; */
/* OBSOLETE   nia = s_idecode_issue(sd, */
/* OBSOLETE 			insn1, */
/* OBSOLETE 			cia); */
/* OBSOLETE  */
/* OBSOLETE   unqueue_writes (sd, STATE_CPU (sd, 0), cia); */
/* OBSOLETE  */
/* OBSOLETE   /* Only do the second instruction if the PC has not changed */ */
/* OBSOLETE   if ((nia == INVALID_INSTRUCTION_ADDRESS) && */
/* OBSOLETE       (! STATE_CPU (sd, 0)->left_kills_right_p)) { */
/* OBSOLETE     STATE_CPU (sd, 0)->unit = any_unit; */
/* OBSOLETE     nia = s_idecode_issue (sd, */
/* OBSOLETE 			   insn2, */
/* OBSOLETE 			   cia); */
/* OBSOLETE  */
/* OBSOLETE     unqueue_writes (sd, STATE_CPU (sd, 0), cia); */
/* OBSOLETE   } */
/* OBSOLETE  */
/* OBSOLETE   STATE_CPU (sd, 0)->left_kills_right_p = 0; */
/* OBSOLETE   STATE_CPU (sd, 0)->mvtsys_left_p = 0; */
/* OBSOLETE   return nia; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static address_word */
/* OBSOLETE do_parallel (SIM_DESC sd, */
/* OBSOLETE 	     s_instruction_word left_insn, */
/* OBSOLETE 	     s_instruction_word right_insn, */
/* OBSOLETE 	     address_word cia) */
/* OBSOLETE { */
/* OBSOLETE   address_word nia_left; */
/* OBSOLETE   address_word nia_right; */
/* OBSOLETE   address_word nia; */
/* OBSOLETE  */
/* OBSOLETE   /* run the first instruction */ */
/* OBSOLETE   STATE_CPU (sd, 0)->unit = memory_unit; */
/* OBSOLETE   STATE_CPU (sd, 0)->left_kills_right_p = 0; */
/* OBSOLETE   STATE_CPU (sd, 0)->mvtsys_left_p = 0; */
/* OBSOLETE   nia_left = s_idecode_issue(sd, */
/* OBSOLETE 			     left_insn, */
/* OBSOLETE 			     cia); */
/* OBSOLETE  */
/* OBSOLETE   /* run the second instruction */ */
/* OBSOLETE   STATE_CPU (sd, 0)->unit = integer_unit; */
/* OBSOLETE   nia_right = s_idecode_issue(sd, */
/* OBSOLETE 			      right_insn, */
/* OBSOLETE 			      cia); */
/* OBSOLETE  */
/* OBSOLETE   /* merge the PC's */ */
/* OBSOLETE   if (nia_left == INVALID_INSTRUCTION_ADDRESS) { */
/* OBSOLETE     if (nia_right == INVALID_INSTRUCTION_ADDRESS) */
/* OBSOLETE       nia = INVALID_INSTRUCTION_ADDRESS; */
/* OBSOLETE     else */
/* OBSOLETE       nia = nia_right; */
/* OBSOLETE   } */
/* OBSOLETE   else { */
/* OBSOLETE     if (nia_right == INVALID_INSTRUCTION_ADDRESS) */
/* OBSOLETE       nia = nia_left; */
/* OBSOLETE     else { */
/* OBSOLETE       sim_engine_abort (sd, STATE_CPU (sd, 0), cia, "parallel jumps"); */
/* OBSOLETE       nia = INVALID_INSTRUCTION_ADDRESS; */
/* OBSOLETE     } */
/* OBSOLETE   } */
/* OBSOLETE  */
/* OBSOLETE   unqueue_writes (sd, STATE_CPU (sd, 0), cia); */
/* OBSOLETE   return nia; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE typedef enum { */
/* OBSOLETE   p_insn = 0, */
/* OBSOLETE   long_insn = 3, */
/* OBSOLETE   l_r_insn = 1, */
/* OBSOLETE   r_l_insn = 2, */
/* OBSOLETE } instruction_types; */
/* OBSOLETE  */
/* OBSOLETE STATIC_INLINE instruction_types */
/* OBSOLETE instruction_type(l_instruction_word insn) */
/* OBSOLETE { */
/* OBSOLETE   int fm0 = MASKED64(insn, 0, 0) != 0; */
/* OBSOLETE   int fm1 = MASKED64(insn, 32, 32) != 0; */
/* OBSOLETE   return ((fm0 << 1) | fm1); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE sim_engine_run (SIM_DESC sd, */
/* OBSOLETE 		int last_cpu_nr, */
/* OBSOLETE 		int nr_cpus, */
/* OBSOLETE 		int siggnal) */
/* OBSOLETE { */
/* OBSOLETE   while (1) */
/* OBSOLETE     { */
/* OBSOLETE       address_word cia = PC; */
/* OBSOLETE       address_word nia; */
/* OBSOLETE       l_instruction_word insn = IMEM(cia); */
/* OBSOLETE       int rp_was_set; */
/* OBSOLETE       int rpt_c_was_nonzero; */
/* OBSOLETE  */
/* OBSOLETE       /* Before executing the instruction, we need to test whether or */
/* OBSOLETE 	 not RPT_C is greater than zero, and save that state for use */
/* OBSOLETE 	 after executing the instruction.  In particular, we need to */
/* OBSOLETE 	 not care whether the instruction changes RPT_C itself. */ */
/* OBSOLETE  */
/* OBSOLETE       rpt_c_was_nonzero = (RPT_C > 0); */
/* OBSOLETE  */
/* OBSOLETE       /* Before executing the instruction, we need to check to see if */
/* OBSOLETE 	 we have to decrement RPT_C, the repeat count register.  Do this */
/* OBSOLETE 	 if PC == RPT_E, but only if we are in an active repeat block. */ */
/* OBSOLETE  */
/* OBSOLETE       if (PC == RPT_E && */
/* OBSOLETE 	  (RPT_C > 0 || PSW_VAL (PSW_RP) != 0)) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  RPT_C --; */
/* OBSOLETE 	} */
/* OBSOLETE        */
/* OBSOLETE       /* Now execute the instruction at PC */ */
/* OBSOLETE  */
/* OBSOLETE       switch (instruction_type (insn)) */
/* OBSOLETE 	{ */
/* OBSOLETE 	case long_insn: */
/* OBSOLETE 	  nia = do_long (sd, insn, cia); */
/* OBSOLETE 	  break; */
/* OBSOLETE 	case r_l_insn: */
/* OBSOLETE 	  /* L <- R */ */
/* OBSOLETE 	  nia = do_2_short (sd, insn, insn >> 32, integer_unit, cia); */
/* OBSOLETE 	  break; */
/* OBSOLETE 	case l_r_insn: */
/* OBSOLETE 	  /* L -> R */ */
/* OBSOLETE 	  nia = do_2_short (sd, insn >> 32, insn, memory_unit, cia); */
/* OBSOLETE 	  break; */
/* OBSOLETE 	case p_insn: */
/* OBSOLETE 	  nia = do_parallel (sd, insn >> 32, insn, cia); */
/* OBSOLETE 	  break; */
/* OBSOLETE 	default: */
/* OBSOLETE 	  sim_engine_abort (sd, STATE_CPU (sd, 0), cia, */
/* OBSOLETE 			    "internal error - engine_run_until_stop - bad switch"); */
/* OBSOLETE 	  nia = -1; */
/* OBSOLETE 	} */
/* OBSOLETE  */
/* OBSOLETE       if (TRACE_ACTION) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  if (TRACE_ACTION & TRACE_ACTION_CALL) */
/* OBSOLETE 	    call_occurred (sd, STATE_CPU (sd, 0), cia, nia); */
/* OBSOLETE  */
/* OBSOLETE 	  if (TRACE_ACTION & TRACE_ACTION_RETURN) */
/* OBSOLETE 	    return_occurred (sd, STATE_CPU (sd, 0), cia, nia); */
/* OBSOLETE  */
/* OBSOLETE 	  TRACE_ACTION = 0; */
/* OBSOLETE 	} */
/* OBSOLETE  */
/* OBSOLETE       /* Check now to see if we need to reset the RP bit in the PSW. */
/* OBSOLETE 	 There are three conditions for this, the RP bit is already */
/* OBSOLETE 	 set (just a speed optimization), the instruction we just */
/* OBSOLETE 	 executed is the last instruction in the loop, and the repeat */
/* OBSOLETE 	 count is currently zero. */ */
/* OBSOLETE  */
/* OBSOLETE       rp_was_set = PSW_VAL (PSW_RP); */
/* OBSOLETE       if (rp_was_set && (PC == RPT_E) && RPT_C == 0) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  PSW_SET (PSW_RP, 0); */
/* OBSOLETE 	} */
/* OBSOLETE  */
/* OBSOLETE       /* Now update the PC.  If we just executed a jump instruction, */
/* OBSOLETE 	 that takes precedence over everything else.  Next comes */
/* OBSOLETE 	 branching back to RPT_S as a result of a loop.  Finally, the */
/* OBSOLETE 	 default is to simply advance to the next inline */
/* OBSOLETE 	 instruction. */ */
/* OBSOLETE  */
/* OBSOLETE       if (nia != INVALID_INSTRUCTION_ADDRESS) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  PC = nia; */
/* OBSOLETE 	} */
/* OBSOLETE       else if (rp_was_set && rpt_c_was_nonzero && (PC == RPT_E)) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  PC = RPT_S; */
/* OBSOLETE 	} */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE 	  PC = cia + 8; */
/* OBSOLETE 	} */
/* OBSOLETE  */
/* OBSOLETE       /* Check for DDBT (debugger debug trap) condition.  Do this after */
/* OBSOLETE 	 the repeat block checks so the excursion to the trap handler does */
/* OBSOLETE 	 not alter looping state. */ */
/* OBSOLETE  */
/* OBSOLETE       if (cia == IBA && PSW_VAL (PSW_DB)) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  DPC = PC; */
/* OBSOLETE 	  PSW_SET (PSW_EA, 1); */
/* OBSOLETE 	  DPSW = PSW; */
/* OBSOLETE 	  /* clear all bits in PSW except SM */ */
/* OBSOLETE 	  PSW &= BIT32 (PSW_SM); */
/* OBSOLETE 	  /* add DS bit */ */
/* OBSOLETE 	  PSW |= BIT32 (PSW_DS); */
/* OBSOLETE 	  /* dispatch to DDBT handler */ */
/* OBSOLETE 	  PC = 0xfffff128; /* debugger_debug_trap_address */ */
/* OBSOLETE 	} */
/* OBSOLETE  */
/* OBSOLETE       /* process any events */ */
/* OBSOLETE       /* FIXME - should L->R or L<-R insns count as two cycles? */ */
/* OBSOLETE       if (sim_events_tick (sd)) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  sim_events_process (sd); */
/* OBSOLETE 	} */
/* OBSOLETE     }   */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* d30v external interrupt handler. */
/* OBSOLETE  */
/* OBSOLETE    Note: This should be replaced by a proper interrupt delivery */
/* OBSOLETE    mechanism.  This interrupt mechanism discards later interrupts if */
/* OBSOLETE    an earlier interrupt hasn't been delivered. */
/* OBSOLETE  */
/* OBSOLETE    Note: This interrupt mechanism does not reset its self when the */
/* OBSOLETE    simulator is re-opened. */ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_interrupt_event (SIM_DESC sd, */
/* OBSOLETE 		      void *data) */
/* OBSOLETE { */
/* OBSOLETE   if (PSW_VAL (PSW_IE)) */
/* OBSOLETE     /* interrupts not masked */ */
/* OBSOLETE     { */
/* OBSOLETE       /* scrub any pending interrupt */ */
/* OBSOLETE       if (sd->pending_interrupt != NULL) */
/* OBSOLETE 	sim_events_deschedule (sd, sd->pending_interrupt); */
/* OBSOLETE       /* deliver */ */
/* OBSOLETE       bPSW = PSW; */
/* OBSOLETE       bPC = PC; */
/* OBSOLETE       PSW = 0; */
/* OBSOLETE       PC = 0xfffff138; /* external interrupt */ */
/* OBSOLETE       do_stack_swap (sd); */
/* OBSOLETE     } */
/* OBSOLETE   else if (sd->pending_interrupt == NULL) */
/* OBSOLETE     /* interrupts masked and no interrupt pending */ */
/* OBSOLETE     { */
/* OBSOLETE       sd->pending_interrupt = sim_events_schedule (sd, 1, */
/* OBSOLETE 						   d30v_interrupt_event, */
/* OBSOLETE 						   data); */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #endif */
