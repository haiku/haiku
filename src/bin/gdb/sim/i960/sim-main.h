/* Main header for the i960.  */

#define USING_SIM_BASE_H /* FIXME: quick hack */

struct _sim_cpu; /* FIXME: should be in sim-basics.h */
typedef struct _sim_cpu SIM_CPU;

#include "symcat.h"
#include "sim-basics.h"
#include "cgen-types.h"
#include "i960-desc.h"
#include "i960-opc.h"
#include "arch.h"

/* These must be defined before sim-base.h.  */
typedef USI sim_cia;
#define CIA_GET(cpu)     0 /* FIXME:(CPU_CGEN_HW (cpu)->h_pc) */
#define CIA_SET(cpu,val) 0 /* FIXME:(CPU_CGEN_HW (cpu)->h_pc = (val)) */

/* FIXME: Shouldn't be required to define these this early.  */
#define SIM_ENGINE_HALT_HOOK(SD, LAST_CPU, CIA)
#define SIM_ENGINE_RESTART_HOOK(SD, LAST_CPU, CIA)

#include "sim-base.h"
#include "cgen-sim.h"
#include "i960-sim.h"

/* The _sim_cpu struct.  */

struct _sim_cpu {
  sim_cpu_base base;

  /* Static parts of cgen.  */
  CGEN_CPU cgen_cpu;

  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  */
#if defined (WANT_CPU_I960BASE)
  I960BASE_CPU_DATA cpu_data;
#endif
};

/* The sim_state struct.  */

struct sim_state {
  sim_cpu *cpu;
#define STATE_CPU(sd, n) (/*&*/ (sd)->cpu)

  CGEN_STATE cgen_state;

  sim_state_base base;
};

/* Misc.  */

/* Catch address exceptions.  */
extern SIM_CORE_SIGNAL_FN i960_core_signal;
#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR) \
i960_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), \
		  (TRANSFER), (ERROR))

/* Default memory size.  */
/* This value comes from the libgloss/i960/mon960.ld linker script.  */
#define I960_DEFAULT_MEM_START 0xa0008000
#define I960_DEFAULT_MEM_SIZE 0x800000 /* 8M */
