// OBSOLETE /* Misc. support for CPU family fr30bf.
// OBSOLETE 
// OBSOLETE THIS FILE IS MACHINE GENERATED WITH CGEN.
// OBSOLETE 
// OBSOLETE Copyright 1996, 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
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
// OBSOLETE 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// OBSOLETE 
// OBSOLETE */
// OBSOLETE 
// OBSOLETE #define WANT_CPU fr30bf
// OBSOLETE #define WANT_CPU_FR30BF
// OBSOLETE 
// OBSOLETE #include "sim-main.h"
// OBSOLETE #include "cgen-ops.h"
// OBSOLETE 
// OBSOLETE /* Get the value of h-pc.  */
// OBSOLETE 
// OBSOLETE USI
// OBSOLETE fr30bf_h_pc_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_pc);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-pc.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_pc_set (SIM_CPU *current_cpu, USI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_pc) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-gr.  */
// OBSOLETE 
// OBSOLETE SI
// OBSOLETE fr30bf_h_gr_get (SIM_CPU *current_cpu, UINT regno)
// OBSOLETE {
// OBSOLETE   return CPU (h_gr[regno]);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-gr.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_gr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_gr[regno]) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-cr.  */
// OBSOLETE 
// OBSOLETE SI
// OBSOLETE fr30bf_h_cr_get (SIM_CPU *current_cpu, UINT regno)
// OBSOLETE {
// OBSOLETE   return CPU (h_cr[regno]);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-cr.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_cr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_cr[regno]) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-dr.  */
// OBSOLETE 
// OBSOLETE SI
// OBSOLETE fr30bf_h_dr_get (SIM_CPU *current_cpu, UINT regno)
// OBSOLETE {
// OBSOLETE   return GET_H_DR (regno);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-dr.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_dr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
// OBSOLETE {
// OBSOLETE   SET_H_DR (regno, newval);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-ps.  */
// OBSOLETE 
// OBSOLETE USI
// OBSOLETE fr30bf_h_ps_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return GET_H_PS ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-ps.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_ps_set (SIM_CPU *current_cpu, USI newval)
// OBSOLETE {
// OBSOLETE   SET_H_PS (newval);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-r13.  */
// OBSOLETE 
// OBSOLETE SI
// OBSOLETE fr30bf_h_r13_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_r13);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-r13.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_r13_set (SIM_CPU *current_cpu, SI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_r13) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-r14.  */
// OBSOLETE 
// OBSOLETE SI
// OBSOLETE fr30bf_h_r14_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_r14);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-r14.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_r14_set (SIM_CPU *current_cpu, SI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_r14) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-r15.  */
// OBSOLETE 
// OBSOLETE SI
// OBSOLETE fr30bf_h_r15_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_r15);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-r15.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_r15_set (SIM_CPU *current_cpu, SI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_r15) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-nbit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_nbit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_nbit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-nbit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_nbit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_nbit) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-zbit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_zbit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_zbit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-zbit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_zbit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_zbit) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-vbit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_vbit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_vbit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-vbit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_vbit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_vbit) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-cbit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_cbit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_cbit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-cbit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_cbit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_cbit) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-ibit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_ibit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_ibit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-ibit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_ibit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_ibit) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-sbit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_sbit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return GET_H_SBIT ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-sbit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_sbit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   SET_H_SBIT (newval);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-tbit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_tbit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_tbit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-tbit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_tbit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_tbit) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-d0bit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_d0bit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_d0bit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-d0bit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_d0bit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_d0bit) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-d1bit.  */
// OBSOLETE 
// OBSOLETE BI
// OBSOLETE fr30bf_h_d1bit_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return CPU (h_d1bit);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-d1bit.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_d1bit_set (SIM_CPU *current_cpu, BI newval)
// OBSOLETE {
// OBSOLETE   CPU (h_d1bit) = newval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-ccr.  */
// OBSOLETE 
// OBSOLETE UQI
// OBSOLETE fr30bf_h_ccr_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return GET_H_CCR ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-ccr.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_ccr_set (SIM_CPU *current_cpu, UQI newval)
// OBSOLETE {
// OBSOLETE   SET_H_CCR (newval);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-scr.  */
// OBSOLETE 
// OBSOLETE UQI
// OBSOLETE fr30bf_h_scr_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return GET_H_SCR ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-scr.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_scr_set (SIM_CPU *current_cpu, UQI newval)
// OBSOLETE {
// OBSOLETE   SET_H_SCR (newval);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Get the value of h-ilm.  */
// OBSOLETE 
// OBSOLETE UQI
// OBSOLETE fr30bf_h_ilm_get (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   return GET_H_ILM ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set a value for h-ilm.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_h_ilm_set (SIM_CPU *current_cpu, UQI newval)
// OBSOLETE {
// OBSOLETE   SET_H_ILM (newval);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Record trace results for INSN.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_record_trace_results (SIM_CPU *current_cpu, CGEN_INSN *insn,
// OBSOLETE 			    int *indices, TRACE_RECORD *tr)
// OBSOLETE {
// OBSOLETE }
