// OBSOLETE /* Simulator model support for fr30bf.
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
// OBSOLETE 
// OBSOLETE /* The profiling data is recorded here, but is accessed via the profiling
// OBSOLETE    mechanism.  After all, this is information for profiling.  */
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE 
// OBSOLETE /* Model handlers for each insn.  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_add (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_addi (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_add2 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_addc (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_addn (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_addni (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_addn2 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_sub (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_subc (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_subn (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_cmp (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_cmpi (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_cmp2 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_and (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_or (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_eor (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_andm (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_andh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_andb (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_orm (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_orh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_orb (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_eorm (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_eorh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_eorb (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bandl (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_borl (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_beorl (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bandh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_borh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_beorh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 1, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 2, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_btstl (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 1, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_btsth (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 1, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_mul (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_mulu (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_mulh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_muluh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_div0s (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_div0u (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_div1 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_div2 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_div3 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_div4s (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_lsl (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_lsli (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_lsl2 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_lsr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_lsri (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_lsr2 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_asr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_asri (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_asr2 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     if (insn_referenced & (1 << 0)) referenced |= 1 << 0;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldi8 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi8.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldi20 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi20.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldi32 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi32.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ld (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_lduh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldub (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr13 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr13uh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr13ub (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr14 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr14uh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14uh.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr14ub (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14ub.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr15 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr15gr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15gr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr15dr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldr15ps (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_st (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_sth (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_stb (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str13 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str13h (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str13b (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str14 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str14h (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14h.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str14b (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14b.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str15 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str15.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str15gr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str15gr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str15dr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_str15ps (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_mov (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_movdr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_movdr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_movps (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_movdr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_mov2dr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_mov2ps (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_jmp (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_jmpd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_callr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_callrd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_call (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_call.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_calld (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_call.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ret (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ret_d (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_int (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_inte (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_reti (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_brad (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bra (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bnod (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bno (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_beqd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_beq (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bned (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bne (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bcd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bc (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bncd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bnc (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bnd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bn (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bpd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bp (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bvd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bv (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bnvd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bnv (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 2)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bltd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 3)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_blt (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 3)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bged (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 3)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bge (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 3)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bled (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 4)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ble (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 4)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bgtd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 4)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bgt (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 4)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_blsd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 3)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bls (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 3)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bhid (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 3)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_bhi (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     if (insn_referenced & (1 << 3)) referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_cti (current_cpu, idesc, 0, referenced, in_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmovr13 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmovr13h (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmovr13b (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 0, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmovr13pi (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmovr13pih (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmovr13pib (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmovr15pi (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr15pi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmov2r13 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmov2r13h (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmov2r13b (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmov2r13pi (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmov2r13pih (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmov2r13pib (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_dmov2r15pd (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr15pi.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldres (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_stres (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_copop (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_copld (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_copst (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_copsv (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_nop (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_andccr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_orccr (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_stilm (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_addsp (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_extsb (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_extub (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_extsh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_extuh (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 2;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldm0 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldm0.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_reglist = 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_ldm (current_cpu, idesc, 0, referenced, in_reglist);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_ldm1 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldm1.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_reglist = 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_ldm (current_cpu, idesc, 0, referenced, in_reglist);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_stm0 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_stm0.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_reglist = 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_stm (current_cpu, idesc, 0, referenced, in_reglist);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_stm1 (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_stm1.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_reglist = 0;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_stm (current_cpu, idesc, 0, referenced, in_reglist);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_enter (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_enter.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_leave (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_enter.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_exec (current_cpu, idesc, 0, referenced, in_Ri, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE model_fr30_1_xchb (SIM_CPU *current_cpu, void *sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
// OBSOLETE   const IDESC * UNUSED idesc = abuf->idesc;
// OBSOLETE   int cycles = 0;
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     INT out_Ri = -1;
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     out_Ri = FLD (out_Ri);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_load (current_cpu, idesc, 0, referenced, in_Rj, out_Ri);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     int referenced = 0;
// OBSOLETE     int UNUSED insn_referenced = abuf->written;
// OBSOLETE     INT in_Ri = -1;
// OBSOLETE     INT in_Rj = -1;
// OBSOLETE     in_Ri = FLD (in_Ri);
// OBSOLETE     in_Rj = FLD (in_Rj);
// OBSOLETE     referenced |= 1 << 0;
// OBSOLETE     referenced |= 1 << 1;
// OBSOLETE     cycles += fr30bf_model_fr30_1_u_store (current_cpu, idesc, 1, referenced, in_Ri, in_Rj);
// OBSOLETE   }
// OBSOLETE   return cycles;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* We assume UNIT_NONE == 0 because the tables don't always terminate
// OBSOLETE    entries with it.  */
// OBSOLETE 
// OBSOLETE /* Model timing data for `fr30-1'.  */
// OBSOLETE 
// OBSOLETE static const INSN_TIMING fr30_1_timing[] = {
// OBSOLETE   { FR30BF_INSN_X_INVALID, 0, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_X_AFTER, 0, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_X_BEFORE, 0, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_X_CTI_CHAIN, 0, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_X_CHAIN, 0, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_X_BEGIN, 0, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ADD, model_fr30_1_add, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ADDI, model_fr30_1_addi, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ADD2, model_fr30_1_add2, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ADDC, model_fr30_1_addc, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ADDN, model_fr30_1_addn, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ADDNI, model_fr30_1_addni, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ADDN2, model_fr30_1_addn2, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_SUB, model_fr30_1_sub, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_SUBC, model_fr30_1_subc, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_SUBN, model_fr30_1_subn, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_CMP, model_fr30_1_cmp, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_CMPI, model_fr30_1_cmpi, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_CMP2, model_fr30_1_cmp2, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_AND, model_fr30_1_and, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_OR, model_fr30_1_or, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_EOR, model_fr30_1_eor, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ANDM, model_fr30_1_andm, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ANDH, model_fr30_1_andh, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ANDB, model_fr30_1_andb, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ORM, model_fr30_1_orm, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ORH, model_fr30_1_orh, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ORB, model_fr30_1_orb, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_EORM, model_fr30_1_eorm, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_EORH, model_fr30_1_eorh, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_EORB, model_fr30_1_eorb, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BANDL, model_fr30_1_bandl, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BORL, model_fr30_1_borl, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BEORL, model_fr30_1_beorl, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BANDH, model_fr30_1_bandh, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BORH, model_fr30_1_borh, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BEORH, model_fr30_1_beorh, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 }, { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BTSTL, model_fr30_1_btstl, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_EXEC, 1, 2 } } },
// OBSOLETE   { FR30BF_INSN_BTSTH, model_fr30_1_btsth, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_EXEC, 1, 2 } } },
// OBSOLETE   { FR30BF_INSN_MUL, model_fr30_1_mul, { { (int) UNIT_FR30_1_U_EXEC, 1, 5 } } },
// OBSOLETE   { FR30BF_INSN_MULU, model_fr30_1_mulu, { { (int) UNIT_FR30_1_U_EXEC, 1, 5 } } },
// OBSOLETE   { FR30BF_INSN_MULH, model_fr30_1_mulh, { { (int) UNIT_FR30_1_U_EXEC, 1, 3 } } },
// OBSOLETE   { FR30BF_INSN_MULUH, model_fr30_1_muluh, { { (int) UNIT_FR30_1_U_EXEC, 1, 3 } } },
// OBSOLETE   { FR30BF_INSN_DIV0S, model_fr30_1_div0s, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DIV0U, model_fr30_1_div0u, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DIV1, model_fr30_1_div1, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DIV2, model_fr30_1_div2, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DIV3, model_fr30_1_div3, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DIV4S, model_fr30_1_div4s, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LSL, model_fr30_1_lsl, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LSLI, model_fr30_1_lsli, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LSL2, model_fr30_1_lsl2, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LSR, model_fr30_1_lsr, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LSRI, model_fr30_1_lsri, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LSR2, model_fr30_1_lsr2, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ASR, model_fr30_1_asr, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ASRI, model_fr30_1_asri, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ASR2, model_fr30_1_asr2, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDI8, model_fr30_1_ldi8, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDI20, model_fr30_1_ldi20, { { (int) UNIT_FR30_1_U_EXEC, 1, 2 } } },
// OBSOLETE   { FR30BF_INSN_LDI32, model_fr30_1_ldi32, { { (int) UNIT_FR30_1_U_EXEC, 1, 3 } } },
// OBSOLETE   { FR30BF_INSN_LD, model_fr30_1_ld, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDUH, model_fr30_1_lduh, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDUB, model_fr30_1_ldub, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR13, model_fr30_1_ldr13, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR13UH, model_fr30_1_ldr13uh, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR13UB, model_fr30_1_ldr13ub, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR14, model_fr30_1_ldr14, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR14UH, model_fr30_1_ldr14uh, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR14UB, model_fr30_1_ldr14ub, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR15, model_fr30_1_ldr15, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR15GR, model_fr30_1_ldr15gr, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR15DR, model_fr30_1_ldr15dr, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDR15PS, model_fr30_1_ldr15ps, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ST, model_fr30_1_st, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STH, model_fr30_1_sth, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STB, model_fr30_1_stb, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR13, model_fr30_1_str13, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR13H, model_fr30_1_str13h, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR13B, model_fr30_1_str13b, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR14, model_fr30_1_str14, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR14H, model_fr30_1_str14h, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR14B, model_fr30_1_str14b, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR15, model_fr30_1_str15, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR15GR, model_fr30_1_str15gr, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR15DR, model_fr30_1_str15dr, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STR15PS, model_fr30_1_str15ps, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_MOV, model_fr30_1_mov, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_MOVDR, model_fr30_1_movdr, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_MOVPS, model_fr30_1_movps, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_MOV2DR, model_fr30_1_mov2dr, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_MOV2PS, model_fr30_1_mov2ps, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_JMP, model_fr30_1_jmp, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_JMPD, model_fr30_1_jmpd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_CALLR, model_fr30_1_callr, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_CALLRD, model_fr30_1_callrd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_CALL, model_fr30_1_call, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_CALLD, model_fr30_1_calld, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_RET, model_fr30_1_ret, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_RET_D, model_fr30_1_ret_d, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_INT, model_fr30_1_int, { { (int) UNIT_FR30_1_U_EXEC, 1, 6 } } },
// OBSOLETE   { FR30BF_INSN_INTE, model_fr30_1_inte, { { (int) UNIT_FR30_1_U_EXEC, 1, 6 } } },
// OBSOLETE   { FR30BF_INSN_RETI, model_fr30_1_reti, { { (int) UNIT_FR30_1_U_EXEC, 1, 4 } } },
// OBSOLETE   { FR30BF_INSN_BRAD, model_fr30_1_brad, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BRA, model_fr30_1_bra, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BNOD, model_fr30_1_bnod, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BNO, model_fr30_1_bno, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BEQD, model_fr30_1_beqd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BEQ, model_fr30_1_beq, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BNED, model_fr30_1_bned, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BNE, model_fr30_1_bne, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BCD, model_fr30_1_bcd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BC, model_fr30_1_bc, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BNCD, model_fr30_1_bncd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BNC, model_fr30_1_bnc, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BND, model_fr30_1_bnd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BN, model_fr30_1_bn, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BPD, model_fr30_1_bpd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BP, model_fr30_1_bp, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BVD, model_fr30_1_bvd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BV, model_fr30_1_bv, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BNVD, model_fr30_1_bnvd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BNV, model_fr30_1_bnv, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BLTD, model_fr30_1_bltd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BLT, model_fr30_1_blt, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BGED, model_fr30_1_bged, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BGE, model_fr30_1_bge, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BLED, model_fr30_1_bled, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BLE, model_fr30_1_ble, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BGTD, model_fr30_1_bgtd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BGT, model_fr30_1_bgt, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BLSD, model_fr30_1_blsd, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BLS, model_fr30_1_bls, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BHID, model_fr30_1_bhid, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_BHI, model_fr30_1_bhi, { { (int) UNIT_FR30_1_U_CTI, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOVR13, model_fr30_1_dmovr13, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOVR13H, model_fr30_1_dmovr13h, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOVR13B, model_fr30_1_dmovr13b, { { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOVR13PI, model_fr30_1_dmovr13pi, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOVR13PIH, model_fr30_1_dmovr13pih, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOVR13PIB, model_fr30_1_dmovr13pib, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOVR15PI, model_fr30_1_dmovr15pi, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOV2R13, model_fr30_1_dmov2r13, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOV2R13H, model_fr30_1_dmov2r13h, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOV2R13B, model_fr30_1_dmov2r13b, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOV2R13PI, model_fr30_1_dmov2r13pi, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOV2R13PIH, model_fr30_1_dmov2r13pih, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOV2R13PIB, model_fr30_1_dmov2r13pib, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_DMOV2R15PD, model_fr30_1_dmov2r15pd, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDRES, model_fr30_1_ldres, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STRES, model_fr30_1_stres, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_COPOP, model_fr30_1_copop, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_COPLD, model_fr30_1_copld, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_COPST, model_fr30_1_copst, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_COPSV, model_fr30_1_copsv, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_NOP, model_fr30_1_nop, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ANDCCR, model_fr30_1_andccr, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ORCCR, model_fr30_1_orccr, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STILM, model_fr30_1_stilm, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ADDSP, model_fr30_1_addsp, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_EXTSB, model_fr30_1_extsb, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_EXTUB, model_fr30_1_extub, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_EXTSH, model_fr30_1_extsh, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_EXTUH, model_fr30_1_extuh, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDM0, model_fr30_1_ldm0, { { (int) UNIT_FR30_1_U_LDM, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_LDM1, model_fr30_1_ldm1, { { (int) UNIT_FR30_1_U_LDM, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STM0, model_fr30_1_stm0, { { (int) UNIT_FR30_1_U_STM, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_STM1, model_fr30_1_stm1, { { (int) UNIT_FR30_1_U_STM, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_ENTER, model_fr30_1_enter, { { (int) UNIT_FR30_1_U_EXEC, 1, 2 } } },
// OBSOLETE   { FR30BF_INSN_LEAVE, model_fr30_1_leave, { { (int) UNIT_FR30_1_U_EXEC, 1, 1 } } },
// OBSOLETE   { FR30BF_INSN_XCHB, model_fr30_1_xchb, { { (int) UNIT_FR30_1_U_LOAD, 1, 1 }, { (int) UNIT_FR30_1_U_STORE, 1, 1 } } },
// OBSOLETE };
// OBSOLETE 
// OBSOLETE #endif /* WITH_PROFILE_MODEL_P */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE fr30_1_model_init (SIM_CPU *cpu)
// OBSOLETE {
// OBSOLETE   CPU_MODEL_DATA (cpu) = (void *) zalloc (sizeof (MODEL_FR30_1_DATA));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE #define TIMING_DATA(td) td
// OBSOLETE #else
// OBSOLETE #define TIMING_DATA(td) 0
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE static const MODEL fr30_models[] =
// OBSOLETE {
// OBSOLETE   { "fr30-1", & fr30_mach, MODEL_FR30_1, TIMING_DATA (& fr30_1_timing[0]), fr30_1_model_init },
// OBSOLETE   { 0 }
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* The properties of this cpu's implementation.  */
// OBSOLETE 
// OBSOLETE static const MACH_IMP_PROPERTIES fr30bf_imp_properties =
// OBSOLETE {
// OBSOLETE   sizeof (SIM_CPU),
// OBSOLETE #if WITH_SCACHE
// OBSOLETE   sizeof (SCACHE)
// OBSOLETE #else
// OBSOLETE   0
// OBSOLETE #endif
// OBSOLETE };
// OBSOLETE 
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE fr30bf_prepare_run (SIM_CPU *cpu)
// OBSOLETE {
// OBSOLETE   if (CPU_IDESC (cpu) == NULL)
// OBSOLETE     fr30bf_init_idesc_table (cpu);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static const CGEN_INSN *
// OBSOLETE fr30bf_get_idata (SIM_CPU *cpu, int inum)
// OBSOLETE {
// OBSOLETE   return CPU_IDESC (cpu) [inum].idata;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE fr30_init_cpu (SIM_CPU *cpu)
// OBSOLETE {
// OBSOLETE   CPU_REG_FETCH (cpu) = fr30bf_fetch_register;
// OBSOLETE   CPU_REG_STORE (cpu) = fr30bf_store_register;
// OBSOLETE   CPU_PC_FETCH (cpu) = fr30bf_h_pc_get;
// OBSOLETE   CPU_PC_STORE (cpu) = fr30bf_h_pc_set;
// OBSOLETE   CPU_GET_IDATA (cpu) = fr30bf_get_idata;
// OBSOLETE   CPU_MAX_INSNS (cpu) = FR30BF_INSN_XCHB + 1;
// OBSOLETE   CPU_INSN_NAME (cpu) = cgen_insn_name;
// OBSOLETE   CPU_FULL_ENGINE_FN (cpu) = fr30bf_engine_run_full;
// OBSOLETE #if WITH_FAST
// OBSOLETE   CPU_FAST_ENGINE_FN (cpu) = fr30bf_engine_run_fast;
// OBSOLETE #else
// OBSOLETE   CPU_FAST_ENGINE_FN (cpu) = fr30bf_engine_run_full;
// OBSOLETE #endif
// OBSOLETE }
// OBSOLETE 
// OBSOLETE const MACH fr30_mach =
// OBSOLETE {
// OBSOLETE   "fr30", "fr30", MACH_FR30,
// OBSOLETE   32, 32, & fr30_models[0], & fr30bf_imp_properties,
// OBSOLETE   fr30_init_cpu,
// OBSOLETE   fr30bf_prepare_run
// OBSOLETE };
