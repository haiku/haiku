/* Simulator model support for i960base.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

This file is part of the GNU Simulators.

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
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#define WANT_CPU i960base
#define WANT_CPU_I960BASE

#include "sim-main.h"

/* The profiling data is recorded here, but is accessed via the profiling
   mechanism.  After all, this is information for profiling.  */

#if WITH_PROFILE_MODEL_P

/* Model handlers for each insn.  */

static int
model_i960KA_mulo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_mulo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_mulo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_mulo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_remo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_remo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_remo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_remo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_divo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_divo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_divo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_divo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_remi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_remi1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_remi2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_remi3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_divi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_divi1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_divi2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_divi3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_addo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_addo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_addo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_addo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_subo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_subo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_subo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_subo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_notbit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_notbit1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_notbit2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_notbit3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_and (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_and1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_and2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_and3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_andnot (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_andnot1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_andnot2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_andnot3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_setbit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_setbit1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_setbit2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_setbit3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_notand (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_notand1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_notand2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_notand3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_xor (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_xor1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_xor2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_xor3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_or (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_or1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_or2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_or3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_nor (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_nor1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_nor2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_nor3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_xnor (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_xnor1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_xnor2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_xnor3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_not (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_not1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_not2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_not3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ornot (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ornot1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ornot2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ornot3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_clrbit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_clrbit1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_clrbit2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_clrbit3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shlo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shlo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shlo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shlo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shro (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shro1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shro2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shro3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shli (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shli1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shli2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shli3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shri (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shri1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shri2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_shri3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_emul (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_emul1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_emul2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_emul3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_mov (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_mov1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_movl (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_movl1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_movt (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_movt1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_movq (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_movq1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_modpc (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_modac (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_lda_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_lda_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_lda_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_lda_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_lda_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_lda_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_lda_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_lda_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ld_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ld_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ld_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ld_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ld_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ld_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ld_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ld_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldob_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldob_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldob_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldob_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldob_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldob_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldob_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldob_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldos_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldos_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldos_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldos_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldos_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldos_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldos_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldos_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldib_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldib_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldib_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldib_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldib_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldib_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldib_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldib_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldis_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldis_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldis_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldis_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldis_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldis_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldis_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldis_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldl_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldl_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldl_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldl_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldl_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldl_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldl_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldl_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldt_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldt_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldt_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldt_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldt_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldt_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldt_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldt_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldq_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldq_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldq_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldq_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldq_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldq_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldq_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ldq_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_st_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_st_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_st_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_st_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_st_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_st_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_st_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_st_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stob_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stob_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stob_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stob_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stob_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stob_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stob_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stob_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stos_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stos_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stos_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stos_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stos_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stos_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stos_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stos_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stl_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stl_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stl_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stl_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stl_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stl_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stl_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stl_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stt_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stt_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stt_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stt_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stt_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stt_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stt_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stt_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stq_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stq_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stq_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stq_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stq_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stq_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stq_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_stq_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobe_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobe_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobne_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobne_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobl_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobl_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpoble_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpoble_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobg_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobg_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobge_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpobge_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibe_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibe_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibne_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibne_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibl_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibl_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpible_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpible_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibg_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibg_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibge_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpibge_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bbc_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bbc_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bbs_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bbs_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpi1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpi2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpi3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_cmpo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_testno_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_testg_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_teste_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_testge_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_testl_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_testne_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_testle_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_testo_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bno (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_be (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bge (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bl (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bne (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ble (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_b (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bx_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bx_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bx_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bx_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_bx_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_callx_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_callx_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_callx_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_ret (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_calls (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_fmark (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960KA_flushreg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960KA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_mulo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_mulo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_mulo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_mulo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_remo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_remo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_remo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_remo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_divo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_divo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_divo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_divo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_remi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_remi1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_remi2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_remi3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_divi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_divi1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_divi2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_divi3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_addo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_addo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_addo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_addo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_subo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_subo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_subo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_subo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_notbit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_notbit1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_notbit2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_notbit3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_and (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_and1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_and2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_and3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_andnot (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_andnot1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_andnot2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_andnot3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_setbit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_setbit1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_setbit2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_setbit3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_notand (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_notand1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_notand2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_notand3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_xor (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_xor1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_xor2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_xor3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_or (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_or1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_or2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_or3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_nor (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_nor1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_nor2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_nor3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_xnor (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_xnor1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_xnor2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_xnor3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_not (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_not1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_not2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_not3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ornot (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ornot1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ornot2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ornot3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_clrbit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_clrbit1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_clrbit2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_clrbit3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shlo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shlo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shlo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shlo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shro (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shro1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shro2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shro3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shli (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shli1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shli2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shli3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shri (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shri1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shri2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_shri3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_emul (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_emul1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_emul2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_emul3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_mov (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_mov1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_movl (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_movl1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_movt (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_movt1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_movq (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_movq1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_modpc (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_modac (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_lda_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_lda_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_lda_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_lda_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_lda_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_lda_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_lda_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_lda_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ld_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ld_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ld_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ld_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ld_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ld_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ld_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ld_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldob_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldob_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldob_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldob_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldob_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldob_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldob_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldob_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldos_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldos_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldos_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldos_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldos_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldos_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldos_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldos_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldib_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldib_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldib_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldib_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldib_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldib_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldib_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldib_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldis_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldis_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldis_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldis_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldis_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldis_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldis_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldis_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldl_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldl_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldl_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldl_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldl_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldl_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldl_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldl_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldt_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldt_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldt_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldt_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldt_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldt_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldt_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldt_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldq_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldq_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldq_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldq_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldq_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldq_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldq_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ldq_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_st_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_st_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_st_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_st_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_st_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_st_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_st_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_st_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stob_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stob_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stob_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stob_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stob_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stob_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stob_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stob_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stos_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stos_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stos_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stos_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stos_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stos_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stos_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stos_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stl_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stl_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stl_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stl_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stl_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stl_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stl_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stl_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stt_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stt_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stt_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stt_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stt_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stt_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stt_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stt_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stq_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stq_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stq_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stq_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stq_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stq_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stq_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_stq_indirect_index_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobe_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobe_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobne_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobne_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobl_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobl_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpoble_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpoble_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobg_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobg_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobge_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpobge_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibe_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibe_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibne_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibne_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibl_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibl_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpible_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpible_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibg_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibg_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibge_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpibge_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bbc_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bbc_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bbs_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bbs_lit (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpi1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpi2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpi3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpo1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpo2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_cmpo3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_testno_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_testg_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_teste_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_testge_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_testl_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_testne_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_testle_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_testo_reg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bno (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_be (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bge (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bl (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bne (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ble (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bo (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_b (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bx_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bx_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bx_indirect_index (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bx_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_bx_indirect_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_callx_disp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_callx_indirect (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_callx_indirect_offset (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_indirect_offset.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_ret (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_disp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_calls (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_fmark (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_i960CA_flushreg (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += i960base_model_i960CA_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

/* We assume UNIT_NONE == 0 because the tables don't always terminate
   entries with it.  */

/* Model timing data for `i960KA'.  */

static const INSN_TIMING i960KA_timing[] = {
  { I960BASE_INSN_X_INVALID, 0, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_AFTER, 0, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_BEFORE, 0, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_CTI_CHAIN, 0, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_CHAIN, 0, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_BEGIN, 0, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MULO, model_i960KA_mulo, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MULO1, model_i960KA_mulo1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MULO2, model_i960KA_mulo2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MULO3, model_i960KA_mulo3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMO, model_i960KA_remo, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMO1, model_i960KA_remo1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMO2, model_i960KA_remo2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMO3, model_i960KA_remo3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVO, model_i960KA_divo, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVO1, model_i960KA_divo1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVO2, model_i960KA_divo2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVO3, model_i960KA_divo3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMI, model_i960KA_remi, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMI1, model_i960KA_remi1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMI2, model_i960KA_remi2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMI3, model_i960KA_remi3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVI, model_i960KA_divi, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVI1, model_i960KA_divi1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVI2, model_i960KA_divi2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVI3, model_i960KA_divi3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ADDO, model_i960KA_addo, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ADDO1, model_i960KA_addo1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ADDO2, model_i960KA_addo2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ADDO3, model_i960KA_addo3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SUBO, model_i960KA_subo, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SUBO1, model_i960KA_subo1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SUBO2, model_i960KA_subo2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SUBO3, model_i960KA_subo3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTBIT, model_i960KA_notbit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTBIT1, model_i960KA_notbit1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTBIT2, model_i960KA_notbit2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTBIT3, model_i960KA_notbit3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_AND, model_i960KA_and, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_AND1, model_i960KA_and1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_AND2, model_i960KA_and2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_AND3, model_i960KA_and3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ANDNOT, model_i960KA_andnot, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ANDNOT1, model_i960KA_andnot1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ANDNOT2, model_i960KA_andnot2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ANDNOT3, model_i960KA_andnot3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SETBIT, model_i960KA_setbit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SETBIT1, model_i960KA_setbit1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SETBIT2, model_i960KA_setbit2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SETBIT3, model_i960KA_setbit3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTAND, model_i960KA_notand, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTAND1, model_i960KA_notand1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTAND2, model_i960KA_notand2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTAND3, model_i960KA_notand3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XOR, model_i960KA_xor, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XOR1, model_i960KA_xor1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XOR2, model_i960KA_xor2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XOR3, model_i960KA_xor3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_OR, model_i960KA_or, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_OR1, model_i960KA_or1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_OR2, model_i960KA_or2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_OR3, model_i960KA_or3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOR, model_i960KA_nor, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOR1, model_i960KA_nor1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOR2, model_i960KA_nor2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOR3, model_i960KA_nor3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XNOR, model_i960KA_xnor, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XNOR1, model_i960KA_xnor1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XNOR2, model_i960KA_xnor2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XNOR3, model_i960KA_xnor3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOT, model_i960KA_not, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOT1, model_i960KA_not1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOT2, model_i960KA_not2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOT3, model_i960KA_not3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ORNOT, model_i960KA_ornot, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ORNOT1, model_i960KA_ornot1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ORNOT2, model_i960KA_ornot2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ORNOT3, model_i960KA_ornot3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CLRBIT, model_i960KA_clrbit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CLRBIT1, model_i960KA_clrbit1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CLRBIT2, model_i960KA_clrbit2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CLRBIT3, model_i960KA_clrbit3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLO, model_i960KA_shlo, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLO1, model_i960KA_shlo1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLO2, model_i960KA_shlo2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLO3, model_i960KA_shlo3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRO, model_i960KA_shro, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRO1, model_i960KA_shro1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRO2, model_i960KA_shro2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRO3, model_i960KA_shro3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLI, model_i960KA_shli, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLI1, model_i960KA_shli1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLI2, model_i960KA_shli2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLI3, model_i960KA_shli3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRI, model_i960KA_shri, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRI1, model_i960KA_shri1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRI2, model_i960KA_shri2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRI3, model_i960KA_shri3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_EMUL, model_i960KA_emul, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_EMUL1, model_i960KA_emul1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_EMUL2, model_i960KA_emul2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_EMUL3, model_i960KA_emul3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOV, model_i960KA_mov, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOV1, model_i960KA_mov1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVL, model_i960KA_movl, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVL1, model_i960KA_movl1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVT, model_i960KA_movt, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVT1, model_i960KA_movt1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVQ, model_i960KA_movq, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVQ1, model_i960KA_movq1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MODPC, model_i960KA_modpc, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MODAC, model_i960KA_modac, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_OFFSET, model_i960KA_lda_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT_OFFSET, model_i960KA_lda_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT, model_i960KA_lda_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT_INDEX, model_i960KA_lda_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_DISP, model_i960KA_lda_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT_DISP, model_i960KA_lda_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDEX_DISP, model_i960KA_lda_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT_INDEX_DISP, model_i960KA_lda_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_OFFSET, model_i960KA_ld_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT_OFFSET, model_i960KA_ld_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT, model_i960KA_ld_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT_INDEX, model_i960KA_ld_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_DISP, model_i960KA_ld_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT_DISP, model_i960KA_ld_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDEX_DISP, model_i960KA_ld_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT_INDEX_DISP, model_i960KA_ld_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_OFFSET, model_i960KA_ldob_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT_OFFSET, model_i960KA_ldob_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT, model_i960KA_ldob_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT_INDEX, model_i960KA_ldob_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_DISP, model_i960KA_ldob_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT_DISP, model_i960KA_ldob_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDEX_DISP, model_i960KA_ldob_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT_INDEX_DISP, model_i960KA_ldob_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_OFFSET, model_i960KA_ldos_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT_OFFSET, model_i960KA_ldos_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT, model_i960KA_ldos_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT_INDEX, model_i960KA_ldos_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_DISP, model_i960KA_ldos_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT_DISP, model_i960KA_ldos_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDEX_DISP, model_i960KA_ldos_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT_INDEX_DISP, model_i960KA_ldos_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_OFFSET, model_i960KA_ldib_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT_OFFSET, model_i960KA_ldib_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT, model_i960KA_ldib_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT_INDEX, model_i960KA_ldib_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_DISP, model_i960KA_ldib_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT_DISP, model_i960KA_ldib_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDEX_DISP, model_i960KA_ldib_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT_INDEX_DISP, model_i960KA_ldib_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_OFFSET, model_i960KA_ldis_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT_OFFSET, model_i960KA_ldis_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT, model_i960KA_ldis_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT_INDEX, model_i960KA_ldis_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_DISP, model_i960KA_ldis_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT_DISP, model_i960KA_ldis_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDEX_DISP, model_i960KA_ldis_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT_INDEX_DISP, model_i960KA_ldis_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_OFFSET, model_i960KA_ldl_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT_OFFSET, model_i960KA_ldl_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT, model_i960KA_ldl_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT_INDEX, model_i960KA_ldl_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_DISP, model_i960KA_ldl_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT_DISP, model_i960KA_ldl_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDEX_DISP, model_i960KA_ldl_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT_INDEX_DISP, model_i960KA_ldl_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_OFFSET, model_i960KA_ldt_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT_OFFSET, model_i960KA_ldt_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT, model_i960KA_ldt_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT_INDEX, model_i960KA_ldt_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_DISP, model_i960KA_ldt_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT_DISP, model_i960KA_ldt_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDEX_DISP, model_i960KA_ldt_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT_INDEX_DISP, model_i960KA_ldt_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_OFFSET, model_i960KA_ldq_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT_OFFSET, model_i960KA_ldq_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT, model_i960KA_ldq_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT_INDEX, model_i960KA_ldq_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_DISP, model_i960KA_ldq_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT_DISP, model_i960KA_ldq_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDEX_DISP, model_i960KA_ldq_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT_INDEX_DISP, model_i960KA_ldq_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_OFFSET, model_i960KA_st_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT_OFFSET, model_i960KA_st_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT, model_i960KA_st_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT_INDEX, model_i960KA_st_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_DISP, model_i960KA_st_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT_DISP, model_i960KA_st_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDEX_DISP, model_i960KA_st_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT_INDEX_DISP, model_i960KA_st_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_OFFSET, model_i960KA_stob_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT_OFFSET, model_i960KA_stob_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT, model_i960KA_stob_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT_INDEX, model_i960KA_stob_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_DISP, model_i960KA_stob_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT_DISP, model_i960KA_stob_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDEX_DISP, model_i960KA_stob_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT_INDEX_DISP, model_i960KA_stob_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_OFFSET, model_i960KA_stos_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT_OFFSET, model_i960KA_stos_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT, model_i960KA_stos_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT_INDEX, model_i960KA_stos_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_DISP, model_i960KA_stos_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT_DISP, model_i960KA_stos_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDEX_DISP, model_i960KA_stos_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT_INDEX_DISP, model_i960KA_stos_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_OFFSET, model_i960KA_stl_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT_OFFSET, model_i960KA_stl_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT, model_i960KA_stl_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT_INDEX, model_i960KA_stl_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_DISP, model_i960KA_stl_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT_DISP, model_i960KA_stl_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDEX_DISP, model_i960KA_stl_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT_INDEX_DISP, model_i960KA_stl_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_OFFSET, model_i960KA_stt_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT_OFFSET, model_i960KA_stt_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT, model_i960KA_stt_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT_INDEX, model_i960KA_stt_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_DISP, model_i960KA_stt_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT_DISP, model_i960KA_stt_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDEX_DISP, model_i960KA_stt_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT_INDEX_DISP, model_i960KA_stt_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_OFFSET, model_i960KA_stq_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT_OFFSET, model_i960KA_stq_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT, model_i960KA_stq_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT_INDEX, model_i960KA_stq_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_DISP, model_i960KA_stq_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT_DISP, model_i960KA_stq_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDEX_DISP, model_i960KA_stq_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT_INDEX_DISP, model_i960KA_stq_indirect_index_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBE_REG, model_i960KA_cmpobe_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBE_LIT, model_i960KA_cmpobe_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBNE_REG, model_i960KA_cmpobne_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBNE_LIT, model_i960KA_cmpobne_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBL_REG, model_i960KA_cmpobl_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBL_LIT, model_i960KA_cmpobl_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBLE_REG, model_i960KA_cmpoble_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBLE_LIT, model_i960KA_cmpoble_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBG_REG, model_i960KA_cmpobg_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBG_LIT, model_i960KA_cmpobg_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBGE_REG, model_i960KA_cmpobge_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBGE_LIT, model_i960KA_cmpobge_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBE_REG, model_i960KA_cmpibe_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBE_LIT, model_i960KA_cmpibe_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBNE_REG, model_i960KA_cmpibne_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBNE_LIT, model_i960KA_cmpibne_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBL_REG, model_i960KA_cmpibl_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBL_LIT, model_i960KA_cmpibl_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBLE_REG, model_i960KA_cmpible_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBLE_LIT, model_i960KA_cmpible_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBG_REG, model_i960KA_cmpibg_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBG_LIT, model_i960KA_cmpibg_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBGE_REG, model_i960KA_cmpibge_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBGE_LIT, model_i960KA_cmpibge_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BBC_REG, model_i960KA_bbc_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BBC_LIT, model_i960KA_bbc_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BBS_REG, model_i960KA_bbs_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BBS_LIT, model_i960KA_bbs_lit, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPI, model_i960KA_cmpi, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPI1, model_i960KA_cmpi1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPI2, model_i960KA_cmpi2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPI3, model_i960KA_cmpi3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPO, model_i960KA_cmpo, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPO1, model_i960KA_cmpo1, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPO2, model_i960KA_cmpo2, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPO3, model_i960KA_cmpo3, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTNO_REG, model_i960KA_testno_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTG_REG, model_i960KA_testg_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTE_REG, model_i960KA_teste_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTGE_REG, model_i960KA_testge_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTL_REG, model_i960KA_testl_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTNE_REG, model_i960KA_testne_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTLE_REG, model_i960KA_testle_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTO_REG, model_i960KA_testo_reg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BNO, model_i960KA_bno, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BG, model_i960KA_bg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BE, model_i960KA_be, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BGE, model_i960KA_bge, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BL, model_i960KA_bl, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BNE, model_i960KA_bne, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BLE, model_i960KA_ble, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BO, model_i960KA_bo, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_B, model_i960KA_b, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_INDIRECT_OFFSET, model_i960KA_bx_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_INDIRECT, model_i960KA_bx_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_INDIRECT_INDEX, model_i960KA_bx_indirect_index, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_DISP, model_i960KA_bx_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_INDIRECT_DISP, model_i960KA_bx_indirect_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CALLX_DISP, model_i960KA_callx_disp, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CALLX_INDIRECT, model_i960KA_callx_indirect, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CALLX_INDIRECT_OFFSET, model_i960KA_callx_indirect_offset, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_RET, model_i960KA_ret, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CALLS, model_i960KA_calls, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_FMARK, model_i960KA_fmark, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_FLUSHREG, model_i960KA_flushreg, { { (int) UNIT_I960KA_U_EXEC, 1, 1 } } },
};

/* Model timing data for `i960CA'.  */

static const INSN_TIMING i960CA_timing[] = {
  { I960BASE_INSN_X_INVALID, 0, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_AFTER, 0, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_BEFORE, 0, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_CTI_CHAIN, 0, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_CHAIN, 0, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_X_BEGIN, 0, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MULO, model_i960CA_mulo, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MULO1, model_i960CA_mulo1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MULO2, model_i960CA_mulo2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MULO3, model_i960CA_mulo3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMO, model_i960CA_remo, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMO1, model_i960CA_remo1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMO2, model_i960CA_remo2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMO3, model_i960CA_remo3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVO, model_i960CA_divo, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVO1, model_i960CA_divo1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVO2, model_i960CA_divo2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVO3, model_i960CA_divo3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMI, model_i960CA_remi, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMI1, model_i960CA_remi1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMI2, model_i960CA_remi2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_REMI3, model_i960CA_remi3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVI, model_i960CA_divi, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVI1, model_i960CA_divi1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVI2, model_i960CA_divi2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_DIVI3, model_i960CA_divi3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ADDO, model_i960CA_addo, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ADDO1, model_i960CA_addo1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ADDO2, model_i960CA_addo2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ADDO3, model_i960CA_addo3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SUBO, model_i960CA_subo, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SUBO1, model_i960CA_subo1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SUBO2, model_i960CA_subo2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SUBO3, model_i960CA_subo3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTBIT, model_i960CA_notbit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTBIT1, model_i960CA_notbit1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTBIT2, model_i960CA_notbit2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTBIT3, model_i960CA_notbit3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_AND, model_i960CA_and, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_AND1, model_i960CA_and1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_AND2, model_i960CA_and2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_AND3, model_i960CA_and3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ANDNOT, model_i960CA_andnot, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ANDNOT1, model_i960CA_andnot1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ANDNOT2, model_i960CA_andnot2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ANDNOT3, model_i960CA_andnot3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SETBIT, model_i960CA_setbit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SETBIT1, model_i960CA_setbit1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SETBIT2, model_i960CA_setbit2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SETBIT3, model_i960CA_setbit3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTAND, model_i960CA_notand, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTAND1, model_i960CA_notand1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTAND2, model_i960CA_notand2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOTAND3, model_i960CA_notand3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XOR, model_i960CA_xor, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XOR1, model_i960CA_xor1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XOR2, model_i960CA_xor2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XOR3, model_i960CA_xor3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_OR, model_i960CA_or, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_OR1, model_i960CA_or1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_OR2, model_i960CA_or2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_OR3, model_i960CA_or3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOR, model_i960CA_nor, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOR1, model_i960CA_nor1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOR2, model_i960CA_nor2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOR3, model_i960CA_nor3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XNOR, model_i960CA_xnor, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XNOR1, model_i960CA_xnor1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XNOR2, model_i960CA_xnor2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_XNOR3, model_i960CA_xnor3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOT, model_i960CA_not, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOT1, model_i960CA_not1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOT2, model_i960CA_not2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_NOT3, model_i960CA_not3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ORNOT, model_i960CA_ornot, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ORNOT1, model_i960CA_ornot1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ORNOT2, model_i960CA_ornot2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ORNOT3, model_i960CA_ornot3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CLRBIT, model_i960CA_clrbit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CLRBIT1, model_i960CA_clrbit1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CLRBIT2, model_i960CA_clrbit2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CLRBIT3, model_i960CA_clrbit3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLO, model_i960CA_shlo, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLO1, model_i960CA_shlo1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLO2, model_i960CA_shlo2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLO3, model_i960CA_shlo3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRO, model_i960CA_shro, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRO1, model_i960CA_shro1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRO2, model_i960CA_shro2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRO3, model_i960CA_shro3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLI, model_i960CA_shli, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLI1, model_i960CA_shli1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLI2, model_i960CA_shli2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHLI3, model_i960CA_shli3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRI, model_i960CA_shri, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRI1, model_i960CA_shri1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRI2, model_i960CA_shri2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_SHRI3, model_i960CA_shri3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_EMUL, model_i960CA_emul, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_EMUL1, model_i960CA_emul1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_EMUL2, model_i960CA_emul2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_EMUL3, model_i960CA_emul3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOV, model_i960CA_mov, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOV1, model_i960CA_mov1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVL, model_i960CA_movl, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVL1, model_i960CA_movl1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVT, model_i960CA_movt, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVT1, model_i960CA_movt1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVQ, model_i960CA_movq, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MOVQ1, model_i960CA_movq1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MODPC, model_i960CA_modpc, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_MODAC, model_i960CA_modac, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_OFFSET, model_i960CA_lda_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT_OFFSET, model_i960CA_lda_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT, model_i960CA_lda_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT_INDEX, model_i960CA_lda_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_DISP, model_i960CA_lda_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT_DISP, model_i960CA_lda_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDEX_DISP, model_i960CA_lda_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDA_INDIRECT_INDEX_DISP, model_i960CA_lda_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_OFFSET, model_i960CA_ld_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT_OFFSET, model_i960CA_ld_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT, model_i960CA_ld_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT_INDEX, model_i960CA_ld_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_DISP, model_i960CA_ld_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT_DISP, model_i960CA_ld_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDEX_DISP, model_i960CA_ld_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LD_INDIRECT_INDEX_DISP, model_i960CA_ld_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_OFFSET, model_i960CA_ldob_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT_OFFSET, model_i960CA_ldob_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT, model_i960CA_ldob_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT_INDEX, model_i960CA_ldob_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_DISP, model_i960CA_ldob_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT_DISP, model_i960CA_ldob_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDEX_DISP, model_i960CA_ldob_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOB_INDIRECT_INDEX_DISP, model_i960CA_ldob_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_OFFSET, model_i960CA_ldos_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT_OFFSET, model_i960CA_ldos_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT, model_i960CA_ldos_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT_INDEX, model_i960CA_ldos_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_DISP, model_i960CA_ldos_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT_DISP, model_i960CA_ldos_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDEX_DISP, model_i960CA_ldos_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDOS_INDIRECT_INDEX_DISP, model_i960CA_ldos_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_OFFSET, model_i960CA_ldib_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT_OFFSET, model_i960CA_ldib_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT, model_i960CA_ldib_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT_INDEX, model_i960CA_ldib_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_DISP, model_i960CA_ldib_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT_DISP, model_i960CA_ldib_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDEX_DISP, model_i960CA_ldib_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIB_INDIRECT_INDEX_DISP, model_i960CA_ldib_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_OFFSET, model_i960CA_ldis_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT_OFFSET, model_i960CA_ldis_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT, model_i960CA_ldis_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT_INDEX, model_i960CA_ldis_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_DISP, model_i960CA_ldis_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT_DISP, model_i960CA_ldis_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDEX_DISP, model_i960CA_ldis_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDIS_INDIRECT_INDEX_DISP, model_i960CA_ldis_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_OFFSET, model_i960CA_ldl_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT_OFFSET, model_i960CA_ldl_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT, model_i960CA_ldl_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT_INDEX, model_i960CA_ldl_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_DISP, model_i960CA_ldl_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT_DISP, model_i960CA_ldl_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDEX_DISP, model_i960CA_ldl_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDL_INDIRECT_INDEX_DISP, model_i960CA_ldl_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_OFFSET, model_i960CA_ldt_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT_OFFSET, model_i960CA_ldt_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT, model_i960CA_ldt_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT_INDEX, model_i960CA_ldt_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_DISP, model_i960CA_ldt_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT_DISP, model_i960CA_ldt_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDEX_DISP, model_i960CA_ldt_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDT_INDIRECT_INDEX_DISP, model_i960CA_ldt_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_OFFSET, model_i960CA_ldq_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT_OFFSET, model_i960CA_ldq_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT, model_i960CA_ldq_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT_INDEX, model_i960CA_ldq_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_DISP, model_i960CA_ldq_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT_DISP, model_i960CA_ldq_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDEX_DISP, model_i960CA_ldq_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_LDQ_INDIRECT_INDEX_DISP, model_i960CA_ldq_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_OFFSET, model_i960CA_st_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT_OFFSET, model_i960CA_st_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT, model_i960CA_st_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT_INDEX, model_i960CA_st_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_DISP, model_i960CA_st_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT_DISP, model_i960CA_st_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDEX_DISP, model_i960CA_st_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_ST_INDIRECT_INDEX_DISP, model_i960CA_st_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_OFFSET, model_i960CA_stob_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT_OFFSET, model_i960CA_stob_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT, model_i960CA_stob_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT_INDEX, model_i960CA_stob_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_DISP, model_i960CA_stob_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT_DISP, model_i960CA_stob_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDEX_DISP, model_i960CA_stob_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOB_INDIRECT_INDEX_DISP, model_i960CA_stob_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_OFFSET, model_i960CA_stos_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT_OFFSET, model_i960CA_stos_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT, model_i960CA_stos_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT_INDEX, model_i960CA_stos_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_DISP, model_i960CA_stos_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT_DISP, model_i960CA_stos_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDEX_DISP, model_i960CA_stos_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STOS_INDIRECT_INDEX_DISP, model_i960CA_stos_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_OFFSET, model_i960CA_stl_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT_OFFSET, model_i960CA_stl_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT, model_i960CA_stl_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT_INDEX, model_i960CA_stl_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_DISP, model_i960CA_stl_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT_DISP, model_i960CA_stl_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDEX_DISP, model_i960CA_stl_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STL_INDIRECT_INDEX_DISP, model_i960CA_stl_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_OFFSET, model_i960CA_stt_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT_OFFSET, model_i960CA_stt_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT, model_i960CA_stt_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT_INDEX, model_i960CA_stt_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_DISP, model_i960CA_stt_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT_DISP, model_i960CA_stt_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDEX_DISP, model_i960CA_stt_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STT_INDIRECT_INDEX_DISP, model_i960CA_stt_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_OFFSET, model_i960CA_stq_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT_OFFSET, model_i960CA_stq_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT, model_i960CA_stq_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT_INDEX, model_i960CA_stq_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_DISP, model_i960CA_stq_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT_DISP, model_i960CA_stq_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDEX_DISP, model_i960CA_stq_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_STQ_INDIRECT_INDEX_DISP, model_i960CA_stq_indirect_index_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBE_REG, model_i960CA_cmpobe_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBE_LIT, model_i960CA_cmpobe_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBNE_REG, model_i960CA_cmpobne_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBNE_LIT, model_i960CA_cmpobne_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBL_REG, model_i960CA_cmpobl_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBL_LIT, model_i960CA_cmpobl_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBLE_REG, model_i960CA_cmpoble_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBLE_LIT, model_i960CA_cmpoble_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBG_REG, model_i960CA_cmpobg_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBG_LIT, model_i960CA_cmpobg_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBGE_REG, model_i960CA_cmpobge_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPOBGE_LIT, model_i960CA_cmpobge_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBE_REG, model_i960CA_cmpibe_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBE_LIT, model_i960CA_cmpibe_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBNE_REG, model_i960CA_cmpibne_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBNE_LIT, model_i960CA_cmpibne_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBL_REG, model_i960CA_cmpibl_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBL_LIT, model_i960CA_cmpibl_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBLE_REG, model_i960CA_cmpible_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBLE_LIT, model_i960CA_cmpible_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBG_REG, model_i960CA_cmpibg_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBG_LIT, model_i960CA_cmpibg_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBGE_REG, model_i960CA_cmpibge_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPIBGE_LIT, model_i960CA_cmpibge_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BBC_REG, model_i960CA_bbc_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BBC_LIT, model_i960CA_bbc_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BBS_REG, model_i960CA_bbs_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BBS_LIT, model_i960CA_bbs_lit, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPI, model_i960CA_cmpi, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPI1, model_i960CA_cmpi1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPI2, model_i960CA_cmpi2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPI3, model_i960CA_cmpi3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPO, model_i960CA_cmpo, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPO1, model_i960CA_cmpo1, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPO2, model_i960CA_cmpo2, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CMPO3, model_i960CA_cmpo3, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTNO_REG, model_i960CA_testno_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTG_REG, model_i960CA_testg_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTE_REG, model_i960CA_teste_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTGE_REG, model_i960CA_testge_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTL_REG, model_i960CA_testl_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTNE_REG, model_i960CA_testne_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTLE_REG, model_i960CA_testle_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_TESTO_REG, model_i960CA_testo_reg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BNO, model_i960CA_bno, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BG, model_i960CA_bg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BE, model_i960CA_be, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BGE, model_i960CA_bge, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BL, model_i960CA_bl, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BNE, model_i960CA_bne, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BLE, model_i960CA_ble, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BO, model_i960CA_bo, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_B, model_i960CA_b, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_INDIRECT_OFFSET, model_i960CA_bx_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_INDIRECT, model_i960CA_bx_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_INDIRECT_INDEX, model_i960CA_bx_indirect_index, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_DISP, model_i960CA_bx_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_BX_INDIRECT_DISP, model_i960CA_bx_indirect_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CALLX_DISP, model_i960CA_callx_disp, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CALLX_INDIRECT, model_i960CA_callx_indirect, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CALLX_INDIRECT_OFFSET, model_i960CA_callx_indirect_offset, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_RET, model_i960CA_ret, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_CALLS, model_i960CA_calls, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_FMARK, model_i960CA_fmark, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
  { I960BASE_INSN_FLUSHREG, model_i960CA_flushreg, { { (int) UNIT_I960CA_U_EXEC, 1, 1 } } },
};

#endif /* WITH_PROFILE_MODEL_P */

static void
i960KA_model_init (SIM_CPU *cpu)
{
  CPU_MODEL_DATA (cpu) = (void *) zalloc (sizeof (MODEL_I960KA_DATA));
}

static void
i960CA_model_init (SIM_CPU *cpu)
{
  CPU_MODEL_DATA (cpu) = (void *) zalloc (sizeof (MODEL_I960CA_DATA));
}

#if WITH_PROFILE_MODEL_P
#define TIMING_DATA(td) td
#else
#define TIMING_DATA(td) 0
#endif

static const MODEL i960_ka_sa_models[] =
{
  { "i960KA", & i960_ka_sa_mach, MODEL_I960KA, TIMING_DATA (& i960KA_timing[0]), i960KA_model_init },
  { 0 }
};

static const MODEL i960_ca_models[] =
{
  { "i960CA", & i960_ca_mach, MODEL_I960CA, TIMING_DATA (& i960CA_timing[0]), i960CA_model_init },
  { 0 }
};

/* The properties of this cpu's implementation.  */

static const MACH_IMP_PROPERTIES i960base_imp_properties =
{
  sizeof (SIM_CPU),
#if WITH_SCACHE
  sizeof (SCACHE)
#else
  0
#endif
};


static void
i960base_prepare_run (SIM_CPU *cpu)
{
  if (CPU_IDESC (cpu) == NULL)
    i960base_init_idesc_table (cpu);
}

static const CGEN_INSN *
i960base_get_idata (SIM_CPU *cpu, int inum)
{
  return CPU_IDESC (cpu) [inum].idata;
}

static void
i960_ka_sa_init_cpu (SIM_CPU *cpu)
{
  CPU_REG_FETCH (cpu) = i960base_fetch_register;
  CPU_REG_STORE (cpu) = i960base_store_register;
  CPU_PC_FETCH (cpu) = i960base_h_pc_get;
  CPU_PC_STORE (cpu) = i960base_h_pc_set;
  CPU_GET_IDATA (cpu) = i960base_get_idata;
  CPU_MAX_INSNS (cpu) = I960BASE_INSN_MAX;
  CPU_INSN_NAME (cpu) = cgen_insn_name;
  CPU_FULL_ENGINE_FN (cpu) = i960base_engine_run_full;
#if WITH_FAST
  CPU_FAST_ENGINE_FN (cpu) = i960base_engine_run_fast;
#else
  CPU_FAST_ENGINE_FN (cpu) = i960base_engine_run_full;
#endif
}

const MACH i960_ka_sa_mach =
{
  "i960:ka_sa", "i960:ka_sa", MACH_I960_KA_SA,
  32, 32, & i960_ka_sa_models[0], & i960base_imp_properties,
  i960_ka_sa_init_cpu,
  i960base_prepare_run
};

static void
i960_ca_init_cpu (SIM_CPU *cpu)
{
  CPU_REG_FETCH (cpu) = i960base_fetch_register;
  CPU_REG_STORE (cpu) = i960base_store_register;
  CPU_PC_FETCH (cpu) = i960base_h_pc_get;
  CPU_PC_STORE (cpu) = i960base_h_pc_set;
  CPU_GET_IDATA (cpu) = i960base_get_idata;
  CPU_MAX_INSNS (cpu) = I960BASE_INSN_MAX;
  CPU_INSN_NAME (cpu) = cgen_insn_name;
  CPU_FULL_ENGINE_FN (cpu) = i960base_engine_run_full;
#if WITH_FAST
  CPU_FAST_ENGINE_FN (cpu) = i960base_engine_run_fast;
#else
  CPU_FAST_ENGINE_FN (cpu) = i960base_engine_run_full;
#endif
}

const MACH i960_ca_mach =
{
  "i960:ca", "i960:ca", MACH_I960_CA,
  32, 32, & i960_ca_models[0], & i960base_imp_properties,
  i960_ca_init_cpu,
  i960base_prepare_run
};

