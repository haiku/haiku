// OBSOLETE /* Main header for the fr30.  */
// OBSOLETE 
// OBSOLETE #define USING_SIM_BASE_H /* FIXME: quick hack */
// OBSOLETE 
// OBSOLETE struct _sim_cpu; /* FIXME: should be in sim-basics.h */
// OBSOLETE typedef struct _sim_cpu SIM_CPU;
// OBSOLETE 
// OBSOLETE /* sim-basics.h includes config.h but cgen-types.h must be included before
// OBSOLETE    sim-basics.h and cgen-types.h needs config.h.  */
// OBSOLETE #include "config.h"
// OBSOLETE 
// OBSOLETE #include "symcat.h"
// OBSOLETE #include "sim-basics.h"
// OBSOLETE #include "cgen-types.h"
// OBSOLETE #include "fr30-desc.h"
// OBSOLETE #include "fr30-opc.h"
// OBSOLETE #include "arch.h"
// OBSOLETE 
// OBSOLETE /* These must be defined before sim-base.h.  */
// OBSOLETE typedef USI sim_cia;
// OBSOLETE 
// OBSOLETE #define CIA_GET(cpu)     CPU_PC_GET (cpu)
// OBSOLETE #define CIA_SET(cpu,val) CPU_PC_SET ((cpu), (val))
// OBSOLETE 
// OBSOLETE #include "sim-base.h"
// OBSOLETE #include "cgen-sim.h"
// OBSOLETE #include "fr30-sim.h"
// OBSOLETE 
// OBSOLETE /* The _sim_cpu struct.  */
// OBSOLETE 
// OBSOLETE struct _sim_cpu {
// OBSOLETE   /* sim/common cpu base.  */
// OBSOLETE   sim_cpu_base base;
// OBSOLETE 
// OBSOLETE   /* Static parts of cgen.  */
// OBSOLETE   CGEN_CPU cgen_cpu;
// OBSOLETE 
// OBSOLETE   /* CPU specific parts go here.
// OBSOLETE      Note that in files that don't need to access these pieces WANT_CPU_FOO
// OBSOLETE      won't be defined and thus these parts won't appear.  This is ok in the
// OBSOLETE      sense that things work.  It is a source of bugs though.
// OBSOLETE      One has to of course be careful to not take the size of this
// OBSOLETE      struct and no structure members accessed in non-cpu specific files can
// OBSOLETE      go after here.  Oh for a better language.  */
// OBSOLETE #if defined (WANT_CPU_FR30BF)
// OBSOLETE   FR30BF_CPU_DATA cpu_data;
// OBSOLETE #endif
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* The sim_state struct.  */
// OBSOLETE 
// OBSOLETE struct sim_state {
// OBSOLETE   sim_cpu *cpu;
// OBSOLETE #define STATE_CPU(sd, n) (/*&*/ (sd)->cpu)
// OBSOLETE 
// OBSOLETE   CGEN_STATE cgen_state;
// OBSOLETE 
// OBSOLETE   sim_state_base base;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Misc.  */
// OBSOLETE 
// OBSOLETE /* Catch address exceptions.  */
// OBSOLETE extern SIM_CORE_SIGNAL_FN fr30_core_signal;
// OBSOLETE #define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR) \
// OBSOLETE fr30_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), \
// OBSOLETE 		  (TRANSFER), (ERROR))
// OBSOLETE 
// OBSOLETE /* Default memory size.  */
// OBSOLETE #define FR30_DEFAULT_MEM_SIZE 0x800000 /* 8M */
