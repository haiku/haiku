#ifndef I960_SIM_H
#define I960_SIM_H

/* gdb register numbers */
/* Copied from gdb/config/i960/tc-i960.h.  */
#define PCW_REGNUM 32   /* process control word */
#define ACW_REGNUM 33   /* arithmetic control word */
#define TCW_REGNUM 34   /* trace control word */
#define IP_REGNUM  35   /* instruction pointer */
#define FP0_REGNUM 36   /* First floating point register */
/* Some registers have more than one name */
#define PC_REGNUM  IP_REGNUM    /* GDB refers to ip as the Program Counter */

SI a_i960_h_gr_get (SIM_CPU *, UINT);
void a_i960_h_gr_set (SIM_CPU *, UINT, SI);
IADDR a_i960_h_pc_get (SIM_CPU *);
void a_i960_h_pc_set (SIM_CPU *, IADDR);

#define GETTWI GETTSI
#define SETTWI SETTSI

/* Exception, Interrupt, and Trap addresses */
/* ??? Hack for traps.c.  */
#define EIT_TRAP_BASE_ADDR	0x40

/* Special purpose traps.  */
/* ??? Hack for traps.c.  */
#define TRAP_SYSCALL	0
#define TRAP_BREAKPOINT	1

/* Cache Purge Control (only exists on early versions of chips) */
/* ??? Hack for devices.c.  */
#define MSPR_ADDR 0xfffffff7
#define MSPR_PURGE 1

/* Cache Control Register */
/* ??? Hack for devices.c.  */
#define MCCR_ADDR 0xffffffff
#define MCCR_CP 0x80

/* Start address and length of all device support.  */
/* ??? Hack for sim-if.c.  */
#define I960_DEVICE_ADDR	0xff000000
#define I960_DEVICE_LEN		0x00ffffff

/* sim_core_attach device argument.  */
/* ??? Hack for sim-if.c.  */
extern device i960_devices;

/* FIXME: Temporary, until device support ready.  */
/* ??? Hack for devices.c.  */
struct _device { int foo; };

#endif I960_SIM_H
