/* OBSOLETE /*  This file is part of the program psim. */
/* OBSOLETE  */
/* OBSOLETE     Copyright (C) 1994-1997, Andrew Cagney <cagney@highland.com.au> */
/* OBSOLETE     Copyright (C) 1997, 1998, Free Software Foundation */
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
/* OBSOLETE #ifndef _SIM_MAIN_H_ */
/* OBSOLETE #define _SIM_MAIN_H_ */
/* OBSOLETE  */
/* OBSOLETE /* This simulator suports watchpoints */ */
/* OBSOLETE #define WITH_WATCHPOINTS 1 */
/* OBSOLETE  */
/* OBSOLETE #include "sim-basics.h" */
/* OBSOLETE #include "sim-signal.h" */
/* OBSOLETE  */
/* OBSOLETE /* needed */ */
/* OBSOLETE typedef address_word sim_cia; */
/* OBSOLETE #define INVALID_INSTRUCTION_ADDRESS ((address_word) 0 - 1) */
/* OBSOLETE  */
/* OBSOLETE /* This simulator doesn't cache anything so no saving of context is */
/* OBSOLETE    needed during either of a halt or restart */ */
/* OBSOLETE #define SIM_ENGINE_HALT_HOOK(SD,CPU,CIA) while (0) */
/* OBSOLETE #define SIM_ENGINE_RESTART_HOOK(SD,CPU,CIA) while (0) */
/* OBSOLETE  */
/* OBSOLETE #include "sim-base.h" */
/* OBSOLETE  */
/* OBSOLETE /* These are generated files.  */ */
/* OBSOLETE #include "itable.h" */
/* OBSOLETE #include "s_idecode.h" */
/* OBSOLETE #include "l_idecode.h" */
/* OBSOLETE  */
/* OBSOLETE #include "cpu.h" */
/* OBSOLETE #include "alu.h" */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE struct sim_state { */
/* OBSOLETE  */
/* OBSOLETE   sim_event *pending_interrupt; */
/* OBSOLETE  */
/* OBSOLETE   /* the processors proper */ */
/* OBSOLETE   sim_cpu cpu[MAX_NR_PROCESSORS]; */
/* OBSOLETE #if (WITH_SMP) */
/* OBSOLETE #define STATE_CPU(sd, n) (&(sd)->cpu[n]) */
/* OBSOLETE #else */
/* OBSOLETE #define STATE_CPU(sd, n) (&(sd)->cpu[0]) */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE   /* The base class.  */ */
/* OBSOLETE   sim_state_base base; */
/* OBSOLETE  */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* deliver an interrupt */ */
/* OBSOLETE sim_event_handler d30v_interrupt_event; */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE #ifdef HAVE_STRING_H */
/* OBSOLETE #include <string.h> */
/* OBSOLETE #else */
/* OBSOLETE #ifdef HAVE_STRINGS_H */
/* OBSOLETE #include <strings.h> */
/* OBSOLETE #endif */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE #endif /* _SIM_MAIN_H_ */ */
