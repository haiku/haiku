// OBSOLETE /* fr30 simulator support code
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
// OBSOLETE #define WANT_CPU
// OBSOLETE #define WANT_CPU_FR30BF
// OBSOLETE 
// OBSOLETE #include "sim-main.h"
// OBSOLETE #include "cgen-mem.h"
// OBSOLETE #include "cgen-ops.h"
// OBSOLETE 
// OBSOLETE /* Convert gdb dedicated register number to actual dr reg number.  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE decode_gdb_dr_regnum (int gdb_regnum)
// OBSOLETE {
// OBSOLETE   switch (gdb_regnum)
// OBSOLETE     {
// OBSOLETE     case TBR_REGNUM : return H_DR_TBR;
// OBSOLETE     case RP_REGNUM : return H_DR_RP;
// OBSOLETE     case SSP_REGNUM : return H_DR_SSP;
// OBSOLETE     case USP_REGNUM : return H_DR_USP;
// OBSOLETE     case MDH_REGNUM : return H_DR_MDH;
// OBSOLETE     case MDL_REGNUM : return H_DR_MDL;
// OBSOLETE     }
// OBSOLETE   abort ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* The contents of BUF are in target byte order.  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30bf_fetch_register (SIM_CPU *current_cpu, int rn, unsigned char *buf, int len)
// OBSOLETE {
// OBSOLETE   if (rn < 16)
// OBSOLETE     SETTWI (buf, fr30bf_h_gr_get (current_cpu, rn));
// OBSOLETE   else
// OBSOLETE     switch (rn)
// OBSOLETE       {
// OBSOLETE       case PC_REGNUM :
// OBSOLETE 	SETTWI (buf, fr30bf_h_pc_get (current_cpu));
// OBSOLETE 	break;
// OBSOLETE       case PS_REGNUM :
// OBSOLETE 	SETTWI (buf, fr30bf_h_ps_get (current_cpu));
// OBSOLETE 	break;
// OBSOLETE       case TBR_REGNUM :
// OBSOLETE       case RP_REGNUM :
// OBSOLETE       case SSP_REGNUM :
// OBSOLETE       case USP_REGNUM :
// OBSOLETE       case MDH_REGNUM :
// OBSOLETE       case MDL_REGNUM :
// OBSOLETE 	SETTWI (buf, fr30bf_h_dr_get (current_cpu,
// OBSOLETE 				      decode_gdb_dr_regnum (rn)));
// OBSOLETE 	break;
// OBSOLETE       default :
// OBSOLETE 	return 0;
// OBSOLETE       }
// OBSOLETE 
// OBSOLETE   return -1; /*FIXME*/
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* The contents of BUF are in target byte order.  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30bf_store_register (SIM_CPU *current_cpu, int rn, unsigned char *buf, int len)
// OBSOLETE {
// OBSOLETE   if (rn < 16)
// OBSOLETE     fr30bf_h_gr_set (current_cpu, rn, GETTWI (buf));
// OBSOLETE   else
// OBSOLETE     switch (rn)
// OBSOLETE       {
// OBSOLETE       case PC_REGNUM :
// OBSOLETE 	fr30bf_h_pc_set (current_cpu, GETTWI (buf));
// OBSOLETE 	break;
// OBSOLETE       case PS_REGNUM :
// OBSOLETE 	fr30bf_h_ps_set (current_cpu, GETTWI (buf));
// OBSOLETE 	break;
// OBSOLETE       case TBR_REGNUM :
// OBSOLETE       case RP_REGNUM :
// OBSOLETE       case SSP_REGNUM :
// OBSOLETE       case USP_REGNUM :
// OBSOLETE       case MDH_REGNUM :
// OBSOLETE       case MDL_REGNUM :
// OBSOLETE 	fr30bf_h_dr_set (current_cpu,
// OBSOLETE 			 decode_gdb_dr_regnum (rn),
// OBSOLETE 			 GETTWI (buf));
// OBSOLETE 	break;
// OBSOLETE       default :
// OBSOLETE 	return 0;
// OBSOLETE       }
// OBSOLETE 
// OBSOLETE   return -1; /*FIXME*/
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Cover fns to access the ccr bits.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_sbit_get_handler (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_sbit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_sbit_set_handler (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   int old_sbit = CPU (h_sbit);
// OBSOLETE   int new_sbit = (newval != 0);
// OBSOLETE 
// OBSOLETE   CPU (h_sbit) = new_sbit;
// OBSOLETE 
// OBSOLETE   /* When switching stack modes, update the registers.  */
// OBSOLETE   if (old_sbit != new_sbit)
// OBSOLETE     {
// OBSOLETE       if (old_sbit)
// OBSOLETE 	{
// OBSOLETE 	  /* Switching user -> system.  */
// OBSOLETE 	  CPU (h_dr[H_DR_USP]) = CPU (h_gr[H_GR_SP]);
// OBSOLETE 	  CPU (h_gr[H_GR_SP]) = CPU (h_dr[H_DR_SSP]);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* Switching system -> user.  */
// OBSOLETE 	  CPU (h_dr[H_DR_SSP]) = CPU (h_gr[H_GR_SP]);
// OBSOLETE 	  CPU (h_gr[H_GR_SP]) = CPU (h_dr[H_DR_USP]);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* TODO: r15 interlock */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Cover fns to access the ccr bits.  */
// OBSOLETE 
// OBSOLETE UQI
// OBSOLETE fr30bf_h_ccr_get_handler (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   int ccr = (  (GET_H_CBIT () << 0)
// OBSOLETE 	     | (GET_H_VBIT () << 1)
// OBSOLETE 	     | (GET_H_ZBIT () << 2)
// OBSOLETE 	     | (GET_H_NBIT () << 3)
// OBSOLETE 	     | (GET_H_IBIT () << 4)
// OBSOLETE 	     | (GET_H_SBIT () << 5));
// OBSOLETE 
// OBSOLETE   return ccr;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_ccr_set_handler (SIM_CPU *current_cpu, UQI newval)
// OBSOLETE {
// OBSOLETE   int ccr = newval & 0x3f;
// OBSOLETE 
// OBSOLETE   SET_H_CBIT ((ccr & 1) != 0);
// OBSOLETE   SET_H_VBIT ((ccr & 2) != 0);
// OBSOLETE   SET_H_ZBIT ((ccr & 4) != 0);
// OBSOLETE   SET_H_NBIT ((ccr & 8) != 0);
// OBSOLETE   SET_H_IBIT ((ccr & 0x10) != 0);
// OBSOLETE   SET_H_SBIT ((ccr & 0x20) != 0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Cover fns to access the scr bits.  */
// OBSOLETE 
// OBSOLETE UQI
// OBSOLETE fr30bf_h_scr_get_handler (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   int scr = (  (GET_H_TBIT () << 0)
// OBSOLETE 	     | (GET_H_D0BIT () << 1)
// OBSOLETE 	     | (GET_H_D1BIT () << 2));
// OBSOLETE   return scr;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_scr_set_handler (SIM_CPU *current_cpu, UQI newval)
// OBSOLETE {
// OBSOLETE   int scr = newval & 7;
// OBSOLETE 
// OBSOLETE   SET_H_TBIT  ((scr & 1) != 0);
// OBSOLETE   SET_H_D0BIT ((scr & 2) != 0);
// OBSOLETE   SET_H_D1BIT ((scr & 4) != 0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Cover fns to access the ilm bits.  */
// OBSOLETE 
// OBSOLETE UQI
// OBSOLETE fr30bf_h_ilm_get_handler (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_ilm);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_ilm_set_handler (SIM_CPU *current_cpu, UQI newval)
// OBSOLETE {
// OBSOLETE   int ilm = newval & 0x1f;
// OBSOLETE   int current_ilm = CPU (h_ilm);
// OBSOLETE 
// OBSOLETE   /* We can only set new ilm values < 16 if the current ilm is < 16.  Otherwise
// OBSOLETE      we add 16 to the value we are given.  */
// OBSOLETE   if (current_ilm >= 16 && ilm < 16)
// OBSOLETE     ilm += 16;
// OBSOLETE 
// OBSOLETE   CPU (h_ilm) = ilm;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Cover fns to access the ps register.  */
// OBSOLETE 
// OBSOLETE USI
// OBSOLETE fr30bf_h_ps_get_handler (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   int ccr = GET_H_CCR ();
// OBSOLETE   int scr = GET_H_SCR ();
// OBSOLETE   int ilm = GET_H_ILM ();
// OBSOLETE 
// OBSOLETE   return ccr | (scr << 8) | (ilm << 16);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_ps_set_handler (SIM_CPU *current_cpu, USI newval)
// OBSOLETE {
// OBSOLETE   int ccr = newval & 0xff;
// OBSOLETE   int scr = (newval >> 8) & 7;
// OBSOLETE   int ilm = (newval >> 16) & 0x1f;
// OBSOLETE 
// OBSOLETE   SET_H_CCR (ccr);
// OBSOLETE   SET_H_SCR (scr);
// OBSOLETE   SET_H_ILM (ilm);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Cover fns to access the dedicated registers.  */
// OBSOLETE 
// OBSOLETE SI
// OBSOLETE fr30bf_h_dr_get_handler (SIM_CPU *current_cpu, UINT dr)
// OBSOLETE {
// OBSOLETE   switch (dr)
// OBSOLETE     {
// OBSOLETE     case H_DR_SSP :
// OBSOLETE       if (! GET_H_SBIT ())
// OBSOLETE 	return GET_H_GR (H_GR_SP);
// OBSOLETE       else
// OBSOLETE 	return CPU (h_dr[H_DR_SSP]);
// OBSOLETE     case H_DR_USP :
// OBSOLETE       if (GET_H_SBIT ())
// OBSOLETE 	return GET_H_GR (H_GR_SP);
// OBSOLETE       else
// OBSOLETE 	return CPU (h_dr[H_DR_USP]);
// OBSOLETE     case H_DR_TBR :
// OBSOLETE     case H_DR_RP :
// OBSOLETE     case H_DR_MDH :
// OBSOLETE     case H_DR_MDL :
// OBSOLETE       return CPU (h_dr[dr]);
// OBSOLETE     }
// OBSOLETE   return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_dr_set_handler (SIM_CPU *current_cpu, UINT dr, SI newval)
// OBSOLETE {
// OBSOLETE   switch (dr)
// OBSOLETE     {
// OBSOLETE     case H_DR_SSP :
// OBSOLETE       if (! GET_H_SBIT ())
// OBSOLETE 	SET_H_GR (H_GR_SP, newval);
// OBSOLETE       else
// OBSOLETE 	CPU (h_dr[H_DR_SSP]) = newval;
// OBSOLETE       break;
// OBSOLETE     case H_DR_USP :
// OBSOLETE       if (GET_H_SBIT ())
// OBSOLETE 	SET_H_GR (H_GR_SP, newval);
// OBSOLETE       else
// OBSOLETE 	CPU (h_dr[H_DR_USP]) = newval;
// OBSOLETE       break;
// OBSOLETE     case H_DR_TBR :
// OBSOLETE     case H_DR_RP :
// OBSOLETE     case H_DR_MDH :
// OBSOLETE     case H_DR_MDL :
// OBSOLETE       CPU (h_dr[dr]) = newval;
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE 
// OBSOLETE /* FIXME: Some of these should be inline or macros.  Later.  */
// OBSOLETE 
// OBSOLETE /* Initialize cycle counting for an insn.
// OBSOLETE    FIRST_P is non-zero if this is the first insn in a set of parallel
// OBSOLETE    insns.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_model_insn_before (SIM_CPU *cpu, int first_p)
// OBSOLETE {
// OBSOLETE   MODEL_FR30_1_DATA *d = CPU_MODEL_DATA (cpu);
// OBSOLETE   d->load_regs_pending = 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Record the cycles computed for an insn.
// OBSOLETE    LAST_P is non-zero if this is the last insn in a set of parallel insns,
// OBSOLETE    and we update the total cycle count.
// OBSOLETE    CYCLES is the cycle count of the insn.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_model_insn_after (SIM_CPU *cpu, int last_p, int cycles)
// OBSOLETE {
// OBSOLETE   PROFILE_DATA *p = CPU_PROFILE_DATA (cpu);
// OBSOLETE   MODEL_FR30_1_DATA *d = CPU_MODEL_DATA (cpu);
// OBSOLETE 
// OBSOLETE   PROFILE_MODEL_TOTAL_CYCLES (p) += cycles;
// OBSOLETE   PROFILE_MODEL_CUR_INSN_CYCLES (p) = cycles;
// OBSOLETE   d->load_regs = d->load_regs_pending;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static INLINE int
// OBSOLETE check_load_stall (SIM_CPU *cpu, int regno)
// OBSOLETE {
// OBSOLETE   const MODEL_FR30_1_DATA *d = CPU_MODEL_DATA (cpu);
// OBSOLETE   UINT load_regs = d->load_regs;
// OBSOLETE 
// OBSOLETE   if (regno != -1
// OBSOLETE       && (load_regs & (1 << regno)) != 0)
// OBSOLETE     {
// OBSOLETE       PROFILE_DATA *p = CPU_PROFILE_DATA (cpu);
// OBSOLETE       ++ PROFILE_MODEL_LOAD_STALL_CYCLES (p);
// OBSOLETE       if (TRACE_INSN_P (cpu))
// OBSOLETE 	cgen_trace_printf (cpu, " ; Load stall.");
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30bf_model_fr30_1_u_exec (SIM_CPU *cpu, const IDESC *idesc,
// OBSOLETE 			    int unit_num, int referenced,
// OBSOLETE 			    INT in_Ri, INT in_Rj, INT out_Ri)
// OBSOLETE {
// OBSOLETE   int cycles = idesc->timing->units[unit_num].done;
// OBSOLETE   cycles += check_load_stall (cpu, in_Ri);
// OBSOLETE   cycles += check_load_stall (cpu, in_Rj);
// OBSOLETE   return cycles;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30bf_model_fr30_1_u_cti (SIM_CPU *cpu, const IDESC *idesc,
// OBSOLETE 			   int unit_num, int referenced,
// OBSOLETE 			   INT in_Ri)
// OBSOLETE {
// OBSOLETE   PROFILE_DATA *p = CPU_PROFILE_DATA (cpu);
// OBSOLETE   /* (1 << 1): The pc is the 2nd element in inputs, outputs.
// OBSOLETE      ??? can be cleaned up */
// OBSOLETE   int taken_p = (referenced & (1 << 1)) != 0;
// OBSOLETE   int cycles = idesc->timing->units[unit_num].done;
// OBSOLETE   int delay_slot_p = CGEN_ATTR_VALUE (NULL, idesc->attrs, CGEN_INSN_DELAY_SLOT);
// OBSOLETE 
// OBSOLETE   cycles += check_load_stall (cpu, in_Ri);
// OBSOLETE   if (taken_p)
// OBSOLETE     {
// OBSOLETE       /* ??? Handling cti's without delay slots this way will run afoul of
// OBSOLETE 	 accurate system simulation.  Later.  */
// OBSOLETE       if (! delay_slot_p)
// OBSOLETE 	{
// OBSOLETE 	  ++cycles;
// OBSOLETE 	  ++PROFILE_MODEL_CTI_STALL_CYCLES (p);
// OBSOLETE 	}
// OBSOLETE       ++PROFILE_MODEL_TAKEN_COUNT (p);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     ++PROFILE_MODEL_UNTAKEN_COUNT (p);
// OBSOLETE 
// OBSOLETE   return cycles;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30bf_model_fr30_1_u_load (SIM_CPU *cpu, const IDESC *idesc,
// OBSOLETE 			    int unit_num, int referenced,
// OBSOLETE 			    INT in_Rj, INT out_Ri)
// OBSOLETE {
// OBSOLETE   MODEL_FR30_1_DATA *d = CPU_MODEL_DATA (cpu);
// OBSOLETE   int cycles = idesc->timing->units[unit_num].done;
// OBSOLETE   d->load_regs_pending |= 1 << out_Ri;
// OBSOLETE   cycles += check_load_stall (cpu, in_Rj);
// OBSOLETE   return cycles;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30bf_model_fr30_1_u_store (SIM_CPU *cpu, const IDESC *idesc,
// OBSOLETE 			     int unit_num, int referenced,
// OBSOLETE 			     INT in_Ri, INT in_Rj)
// OBSOLETE {
// OBSOLETE   int cycles = idesc->timing->units[unit_num].done;
// OBSOLETE   cycles += check_load_stall (cpu, in_Ri);
// OBSOLETE   cycles += check_load_stall (cpu, in_Rj);
// OBSOLETE   return cycles;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30bf_model_fr30_1_u_ldm (SIM_CPU *cpu, const IDESC *idesc,
// OBSOLETE 			   int unit_num, int referenced,
// OBSOLETE 			   INT reglist)
// OBSOLETE {
// OBSOLETE   return idesc->timing->units[unit_num].done;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE fr30bf_model_fr30_1_u_stm (SIM_CPU *cpu, const IDESC *idesc,
// OBSOLETE 			   int unit_num, int referenced,
// OBSOLETE 			   INT reglist)
// OBSOLETE {
// OBSOLETE   return idesc->timing->units[unit_num].done;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #endif /* WITH_PROFILE_MODEL_P */
