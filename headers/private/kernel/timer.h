#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H

/**
 * @file kernel/timer.h
 * @brief Timer structures and definitions
 */

#include <stage2.h>
#include <KernelExport.h>

/**
 * @defgroup Timer Timer structures (not architecture specific)
 * @ingroup OpenBeOS_Kernel
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** kernel functions */
int  timer_init(kernel_args *);
int  timer_interrupt(void);

/** these two are only to be used by the scheduler */
int local_timer_cancel_event(timer *event);
int _local_timer_cancel_event(int curr_cpu, timer *event);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* _KERNEL_TIMER_H */

