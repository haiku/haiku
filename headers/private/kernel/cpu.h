#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H

/**
 * @file kernel/cpu.h
 * @brief CPU Local per-CPU data structure
 */

#include <stage2.h>
#include <smp.h>
#include <timer.h>

/**
 * @defgroup CPU CPU Structures (not architecture specific)
 * @ingroup OpenBeOS_Kernel
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * CPU local data structure
 */
/**
 * One structure per cpu. 
 * The structure contains data that is local to the CPU,
 * helping reduce the amount of cache thrashing.
 */
typedef union cpu_ent {
	/** The information structure, followed by alignment bytes to make it ?? bytes */
	struct {
		/** Number of this CPU, starting from 0 */
		int cpu_num;
		/** If set this will force a reschedule when the quantum timer expires */
		int preempted;
		/** Quantum timer */
		struct timer_event quantum_timer;
	} info;
	/** Alignment bytes */
	uint32 align[16];
} cpu_ent;

/**
 * Defined in core/cpu.c
 */
extern cpu_ent cpu[MAX_BOOT_CPUS];

/**
 * Perform pre-boot initialisation
 * @param ka The kernel_args structure
 */
int cpu_preboot_init(kernel_args *ka);
/**
 * Initialise a CPU
 * @param ka The kernel_args structure
 */
int cpu_init(kernel_args *ka);

/**
 * Get a pointer to the CPU's local data structure
 */
/**
 * This is declared as an inline function in this header
 */
cpu_ent *get_cpu_struct(void);

/**
 * Inline declaration of get_cpu_struct(void)
 */
extern inline cpu_ent *get_cpu_struct(void) { return &cpu[smp_get_current_cpu()]; }

#ifdef __cplusplus
}
#endif
/** @} */

#endif /* _KERNEL_CPU_H */
