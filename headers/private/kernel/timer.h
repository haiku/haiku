#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H

/**
 * @file kernel/timer.h
 * @brief Timer structures and definitions
 */

#include <stage2.h>

/**
 * @defgroup Timer Timer structures (not architecture specific)
 * @ingroup OpenBeOS_Kernel
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct timer timer;
typedef struct qent	qent;
typedef	int32 (*timer_hook)(timer *);

/**
 * qent structure
 */
/**
 * The BeOS qent structure is probably part of a general double linked list
 * interface used all over the kernel; a struct is required to have a qent
 * entry struct as first element, so it can be linked to other elements
 * easily. The key field is probably just an id, eventually used to order
 * the list.
 * Since we don't use this kind of interface, but we have to provide it
 * to keep compatibility, we can use the qent struct for other purposes...
 */
struct qent {
	int64		key;			/* We use this as the sched time */
	qent		*next;			/* This is used as a pointer to next timer */
	qent		*prev;			/* This can be used for callback args */
};

/**
 * Timer event data structure
 */
struct timer {
	/** qent entry for this timer event */
	qent			entry;
	/** This just holds the timer operating mode (relative/absolute/periodic) */
	uint16			flags;
	/** Number of the CPU associated to this timer event */
	uint16			cpu;
	/** Hook function to be called on timer expiration */
	timer_hook		hook;
	/** Expiration time in microseconds */
	bigtime_t		period;
};

/** @defgroup timermodes Timer operating modes
 *  @ingroup Timer
 *  @{
 */
/** @def B_ONE_SHOT_ABSOLUTE_TIMER the timer will fire once at the system
 *  time specified by period
 */
/** @def B_ONE_SHOT_RELATIVE_TIMER the timer will fire once in approximately
 *  period microseconds
 */
/** @def B_PERIODIC_TIMER the timer will fire every period microseconds,
 *  starting in period microseconds
 */
#define		B_ONE_SHOT_ABSOLUTE_TIMER		1
#define		B_ONE_SHOT_RELATIVE_TIMER		2
#define		B_PERIODIC_TIMER				3
/** @} */

status_t  add_timer(timer *event, timer_hook hook, bigtime_t period, int32 flags);
bool      cancel_timer(timer *event);
void      spin(bigtime_t microseconds);

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

