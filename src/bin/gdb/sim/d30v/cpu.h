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
/* OBSOLETE #ifndef _CPU_H_ */
/* OBSOLETE #define _CPU_H_ */
/* OBSOLETE  */
/* OBSOLETE enum { */
/* OBSOLETE   NR_GENERAL_PURPOSE_REGISTERS = 64, */
/* OBSOLETE   NR_CONTROL_REGISTERS = 64, */
/* OBSOLETE   NR_ACCUMULATORS = 2, */
/* OBSOLETE   STACK_POINTER_GPR = 63, */
/* OBSOLETE   NR_STACK_POINTERS = 2, */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE enum { */
/* OBSOLETE   processor_status_word_cr = 0, */
/* OBSOLETE   backup_processor_status_word_cr = 1, */
/* OBSOLETE   program_counter_cr = 2, */
/* OBSOLETE   backup_program_counter_cr = 3, */
/* OBSOLETE   debug_backup_processor_status_word_cr = 4, */
/* OBSOLETE   debug_backup_program_counter_cr = 5, */
/* OBSOLETE   reserved_6_cr = 6, */
/* OBSOLETE   repeat_count_cr = 7, */
/* OBSOLETE   repeat_start_address_cr = 8, */
/* OBSOLETE   repeat_end_address_cr = 9, */
/* OBSOLETE   modulo_start_address_cr = 10, */
/* OBSOLETE   modulo_end_address_cr = 11, */
/* OBSOLETE   instruction_break_address_cr = 14, */
/* OBSOLETE   eit_vector_base_cr = 15, */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE enum { */
/* OBSOLETE   PSW_SM = 0, */
/* OBSOLETE   PSW_EA = 2, */
/* OBSOLETE   PSW_DB = 3, */
/* OBSOLETE   PSW_DS = 4, */
/* OBSOLETE   PSW_IE = 5, */
/* OBSOLETE   PSW_RP = 6, */
/* OBSOLETE   PSW_MD = 7, */
/* OBSOLETE   PSW_F0 = 17, */
/* OBSOLETE   PSW_F1 = 19, */
/* OBSOLETE   PSW_F2 = 21, */
/* OBSOLETE   PSW_F3 = 23, */
/* OBSOLETE   PSW_S = 25, */
/* OBSOLETE   PSW_V = 27, */
/* OBSOLETE   PSW_VA = 29, */
/* OBSOLETE   PSW_C = 31, */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE /* aliases for PSW flag numbers (F0..F7) */ */
/* OBSOLETE enum */
/* OBSOLETE { */
/* OBSOLETE   PSW_S_FLAG = 4, */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE typedef struct _registers { */
/* OBSOLETE   unsigned32 general_purpose[NR_GENERAL_PURPOSE_REGISTERS]; */
/* OBSOLETE   /* keep track of the stack pointer */ */
/* OBSOLETE   unsigned32 sp[NR_STACK_POINTERS]; /* swap with SP */ */
/* OBSOLETE   unsigned32 current_sp; */
/* OBSOLETE   unsigned32 control[NR_CONTROL_REGISTERS]; */
/* OBSOLETE   unsigned64 accumulator[NR_ACCUMULATORS]; */
/* OBSOLETE } registers; */
/* OBSOLETE  */
/* OBSOLETE typedef enum _cpu_units { */
/* OBSOLETE   memory_unit, */
/* OBSOLETE   integer_unit, */
/* OBSOLETE   any_unit, */
/* OBSOLETE } cpu_units; */
/* OBSOLETE  */
/* OBSOLETE /* In order to support parallel instructions, which one instruction can be */
/* OBSOLETE    writing to a register that is used as input to another, queue up the */
/* OBSOLETE    writes to the end of the instruction boundaries.  */ */
/* OBSOLETE  */
/* OBSOLETE #define MAX_WRITE32	16 */
/* OBSOLETE #define MAX_WRITE64	2 */
/* OBSOLETE  */
/* OBSOLETE struct _write32 { */
/* OBSOLETE   int num;				/* # of 32-bit writes queued up */ */
/* OBSOLETE   unsigned32 value[MAX_WRITE32];	/* value to write */ */
/* OBSOLETE   unsigned32 mask[MAX_WRITE32];		/* mask to use */ */
/* OBSOLETE   unsigned32 *ptr[MAX_WRITE32];		/* address to write to */ */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE struct _write64 { */
/* OBSOLETE   int num;				/* # of 64-bit writes queued up */ */
/* OBSOLETE   unsigned64 value[MAX_WRITE64];	/* value to write */ */
/* OBSOLETE   unsigned64 *ptr[MAX_WRITE64];		/* address to write to */ */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE struct _sim_cpu { */
/* OBSOLETE   cpu_units unit; */
/* OBSOLETE   registers regs; */
/* OBSOLETE   sim_cpu_base base; */
/* OBSOLETE   int trace_call_p;			/* Whether to do call tracing.  */ */
/* OBSOLETE   int trace_trap_p;			/* If unknown traps dump out the regs */ */
/* OBSOLETE   int trace_action;			/* trace bits at end of instructions */ */
/* OBSOLETE   int left_kills_right_p;               /* left insn kills insn in right slot of -> */ */
/* OBSOLETE   int mvtsys_left_p;			/* left insn was mvtsys */ */
/* OBSOLETE   int did_trap;				/* we did a trap & need to finish it */ */
/* OBSOLETE   struct _write32 write32;		/* queued up 32-bit writes */ */
/* OBSOLETE   struct _write64 write64;		/* queued up 64-bit writes */ */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE #define PC	(STATE_CPU (sd, 0)->regs.control[program_counter_cr]) */
/* OBSOLETE #define PSW 	(STATE_CPU (sd, 0)->regs.control[processor_status_word_cr]) */
/* OBSOLETE #define PSWL    (*AL2_4(&PSW)) */
/* OBSOLETE #define PSWH    (*AH2_4(&PSW)) */
/* OBSOLETE #define DPSW 	(STATE_CPU (sd, 0)->regs.control[debug_backup_processor_status_word_cr]) */
/* OBSOLETE #define DPC 	(STATE_CPU (sd, 0)->regs.control[debug_backup_program_counter_cr]) */
/* OBSOLETE #define bPC 	(STATE_CPU (sd, 0)->regs.control[backup_program_counter_cr]) */
/* OBSOLETE #define bPSW 	(STATE_CPU (sd, 0)->regs.control[backup_processor_status_word_cr]) */
/* OBSOLETE #define RPT_C 	(STATE_CPU (sd, 0)->regs.control[repeat_count_cr]) */
/* OBSOLETE #define RPT_S 	(STATE_CPU (sd, 0)->regs.control[repeat_start_address_cr]) */
/* OBSOLETE #define RPT_E 	(STATE_CPU (sd, 0)->regs.control[repeat_end_address_cr]) */
/* OBSOLETE #define MOD_S 	(STATE_CPU (sd, 0)->regs.control[modulo_start_address_cr]) */
/* OBSOLETE #define MOD_E 	(STATE_CPU (sd, 0)->regs.control[modulo_end_address_cr]) */
/* OBSOLETE #define IBA 	(STATE_CPU (sd, 0)->regs.control[instruction_break_address_cr]) */
/* OBSOLETE #define EIT_VB	(STATE_CPU (sd, 0)->regs.control[eit_vector_base_cr]) */
/* OBSOLETE #define GPR	(STATE_CPU (sd, 0)->regs.general_purpose) */
/* OBSOLETE #define GPR_CLEAR(N) (GPR[(N)] = 0) */
/* OBSOLETE #define ACC	(STATE_CPU (sd, 0)->regs.accumulator) */
/* OBSOLETE #define CREG	(STATE_CPU (sd, 0)->regs.control) */
/* OBSOLETE #define SP      (GPR[STACK_POINTER_GPR]) */
/* OBSOLETE #define TRACE_CALL_P (STATE_CPU (sd, 0)->trace_call_p) */
/* OBSOLETE #define TRACE_TRAP_P (STATE_CPU (sd, 0)->trace_trap_p) */
/* OBSOLETE #define TRACE_ACTION (STATE_CPU (sd, 0)->trace_action) */
/* OBSOLETE #define     TRACE_ACTION_CALL	0x00000001	/* call occurred */ */
/* OBSOLETE #define     TRACE_ACTION_RETURN	0x00000002	/* return occurred */ */
/* OBSOLETE  */
/* OBSOLETE #define WRITE32 (STATE_CPU (sd, 0)->write32) */
/* OBSOLETE #define WRITE32_NUM	 (WRITE32.num) */
/* OBSOLETE #define WRITE32_PTR(N)	 (WRITE32.ptr[N]) */
/* OBSOLETE #define WRITE32_MASK(N)	 (WRITE32.mask[N]) */
/* OBSOLETE #define WRITE32_VALUE(N) (WRITE32.value[N]) */
/* OBSOLETE #define WRITE32_QUEUE(PTR, VALUE) WRITE32_QUEUE_MASK (PTR, VALUE, 0xffffffff) */
/* OBSOLETE  */
/* OBSOLETE #define WRITE32_QUEUE_MASK(PTR, VALUE, MASK)				\ */
/* OBSOLETE do {									\ */
/* OBSOLETE   int _num = WRITE32_NUM;						\ */
/* OBSOLETE   if (_num >= MAX_WRITE32)						\ */
/* OBSOLETE     sim_engine_abort (sd, STATE_CPU (sd, 0), cia,			\ */
/* OBSOLETE 		      "Too many queued 32-bit writes");			\ */
/* OBSOLETE   WRITE32_PTR(_num) = PTR;						\ */
/* OBSOLETE   WRITE32_VALUE(_num) = VALUE;						\ */
/* OBSOLETE   WRITE32_MASK(_num) = MASK;						\ */
/* OBSOLETE   WRITE32_NUM = _num+1;							\ */
/* OBSOLETE } while (0) */
/* OBSOLETE  */
/* OBSOLETE #define DID_TRAP	(STATE_CPU (sd, 0)->did_trap) */
/* OBSOLETE  */
/* OBSOLETE #define WRITE64 (STATE_CPU (sd, 0)->write64) */
/* OBSOLETE #define WRITE64_NUM	 (WRITE64.num) */
/* OBSOLETE #define WRITE64_PTR(N)	 (WRITE64.ptr[N]) */
/* OBSOLETE #define WRITE64_VALUE(N) (WRITE64.value[N]) */
/* OBSOLETE #define WRITE64_QUEUE(PTR, VALUE)					\ */
/* OBSOLETE do {									\ */
/* OBSOLETE   int _num = WRITE64_NUM;						\ */
/* OBSOLETE   if (_num >= MAX_WRITE64)						\ */
/* OBSOLETE     sim_engine_abort (sd, STATE_CPU (sd, 0), cia,			\ */
/* OBSOLETE 		      "Too many queued 64-bit writes");			\ */
/* OBSOLETE   WRITE64_PTR(_num) = PTR;						\ */
/* OBSOLETE   WRITE64_VALUE(_num) = VALUE;						\ */
/* OBSOLETE   WRITE64_NUM = _num+1;							\ */
/* OBSOLETE } while (0) */
/* OBSOLETE  */
/* OBSOLETE #define DPSW_VALID	0xbf005555 */
/* OBSOLETE #define PSW_VALID	0xb7005555 */
/* OBSOLETE #define EIT_VALID	0xfffff000	/* From page 7-4 of D30V/MPEG arch. manual  */ */
/* OBSOLETE #define EIT_VB_DEFAULT	0xfffff000	/* Value of the EIT_VB register after reset */ */
/* OBSOLETE  */
/* OBSOLETE /* Verify that the instruction is in the correct slot */ */
/* OBSOLETE  */
/* OBSOLETE #define IS_WRONG_SLOT is_wrong_slot(sd, cia, MY_INDEX) */
/* OBSOLETE extern int is_wrong_slot */
/* OBSOLETE (SIM_DESC sd, */
/* OBSOLETE  address_word cia, */
/* OBSOLETE  itable_index index); */
/* OBSOLETE  */
/* OBSOLETE #define IS_CONDITION_OK is_condition_ok(sd, cia, CCC) */
/* OBSOLETE extern int is_condition_ok */
/* OBSOLETE (SIM_DESC sd, */
/* OBSOLETE  address_word cia, */
/* OBSOLETE  int cond); */
/* OBSOLETE  */
/* OBSOLETE #define SIM_HAVE_BREAKPOINTS	/* Turn on internal breakpoint module */ */
/* OBSOLETE  */
/* OBSOLETE /* Internal breakpoint instruction is syscall 5 */ */
/* OBSOLETE #define SIM_BREAKPOINT {0x0e, 0x00, 0x00, 0x05} */
/* OBSOLETE #define SIM_BREAKPOINT_SIZE (4) */
/* OBSOLETE  */
/* OBSOLETE /* Call occurred */ */
/* OBSOLETE extern void call_occurred */
/* OBSOLETE (SIM_DESC sd, */
/* OBSOLETE  sim_cpu *cpu, */
/* OBSOLETE  address_word cia, */
/* OBSOLETE  address_word nia); */
/* OBSOLETE  */
/* OBSOLETE /* Return occurred */ */
/* OBSOLETE extern void return_occurred */
/* OBSOLETE (SIM_DESC sd, */
/* OBSOLETE  sim_cpu *cpu, */
/* OBSOLETE  address_word cia, */
/* OBSOLETE  address_word nia); */
/* OBSOLETE  */
/* OBSOLETE /* Whether to do call tracing.  */ */
/* OBSOLETE extern int d30v_call_trace_p; */
/* OBSOLETE  */
/* OBSOLETE /* Read/write functions for system call interface.  */ */
/* OBSOLETE extern int d30v_read_mem */
/* OBSOLETE (host_callback *cb, */
/* OBSOLETE  struct cb_syscall *sc, */
/* OBSOLETE  unsigned long taddr, */
/* OBSOLETE  char *buf, */
/* OBSOLETE  int bytes); */
/* OBSOLETE  */
/* OBSOLETE extern int d30v_write_mem */
/* OBSOLETE (host_callback *cb, */
/* OBSOLETE  struct cb_syscall *sc, */
/* OBSOLETE  unsigned long taddr, */
/* OBSOLETE  const char *buf, */
/* OBSOLETE  int bytes); */
/* OBSOLETE  */
/* OBSOLETE /* Process all of the queued up writes in order now */ */
/* OBSOLETE void unqueue_writes */
/* OBSOLETE (SIM_DESC sd, */
/* OBSOLETE  sim_cpu *cpu, */
/* OBSOLETE  address_word cia); */
/* OBSOLETE  */
/* OBSOLETE #endif /* _CPU_H_ */ */
