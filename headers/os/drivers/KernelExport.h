/*
 * Copyright 2005-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_EXPORT_H
#define _KERNEL_EXPORT_H


#include <SupportDefs.h>
#include <OS.h>


/* interrupts and spinlocks */

typedef ulong cpu_status;

// WARNING: For Haiku debugging only! This changes the spinlock type in a
// binary incompatible way!
//#define B_DEBUG_SPINLOCK_CONTENTION	1

#if B_DEBUG_SPINLOCK_CONTENTION
	typedef struct {
		vint32	lock;
		vint32	count_low;
		vint32	count_high;
	} spinlock;

#	define B_SPINLOCK_INITIALIZER { 0, 0, 0 }
#	define B_INITIALIZE_SPINLOCK(spinlock)	do {	\
			(spinlock)->lock = 0;					\
			(spinlock)->count_low = 0;				\
			(spinlock)->count_high = 0;				\
		} while (false)
#	define B_SPINLOCK_IS_LOCKED(spinlock)	((spinlock)->lock > 0)
#else
	typedef vint32 spinlock;

#	define B_SPINLOCK_INITIALIZER 0
#	define B_INITIALIZE_SPINLOCK(lock)	do { *(lock) = 0; } while (false)
#	define B_SPINLOCK_IS_LOCKED(lock)	(*(lock) > 0)
#endif

/* interrupt handling support for device drivers */

typedef int32 (*interrupt_handler)(void *data);

/* Values returned by interrupt handlers */
#define B_UNHANDLED_INTERRUPT	0	/* pass to next handler */
#define B_HANDLED_INTERRUPT		1	/* don't pass on */
#define B_INVOKE_SCHEDULER		2	/* don't pass on; invoke the scheduler */

/* Flags that can be passed to install_io_interrupt_handler() */
#define B_NO_ENABLE_COUNTER		1


/* timer interrupts services */

typedef struct timer timer;
typedef	int32 (*timer_hook)(timer *);

struct timer {
	struct timer *next;
	int64		schedule_time;
	void		*user_data;
	uint16		flags;
	uint16		cpu;
	timer_hook	hook;
	bigtime_t	period;
};

#define B_ONE_SHOT_ABSOLUTE_TIMER	1
#define	B_ONE_SHOT_RELATIVE_TIMER	2
#define	B_PERIODIC_TIMER			3


/* virtual memory buffer functions */

#define B_DMA_IO		0x00000001
#define B_READ_DEVICE	0x00000002

typedef struct {
	phys_addr_t	address;	/* address in physical memory */
	phys_size_t	size;		/* size of block */
} physical_entry;

/* address specifications for mapping physical memory */
#define	B_ANY_KERNEL_BLOCK_ADDRESS	(B_ANY_KERNEL_ADDRESS + 1)

/* area protection flags for the kernel */
#define B_KERNEL_READ_AREA			16
#define B_KERNEL_WRITE_AREA			32
#define B_USER_CLONEABLE_AREA		256

/* MTR attributes for mapping physical memory (Intel Architecture only) */
// TODO: rename those to something more meaningful
#define	B_MTR_UC	0x10000000
#define	B_MTR_WC	0x20000000
#define	B_MTR_WT	0x30000000
#define	B_MTR_WP	0x40000000
#define	B_MTR_WB	0x50000000
#define	B_MTR_MASK	0xf0000000


/* kernel daemon service */

typedef void (*daemon_hook)(void *arg, int iteration);


/* kernel debugging facilities */

/* special return codes for kernel debugger */
#define  B_KDEBUG_CONT   2
#define  B_KDEBUG_QUIT   3

typedef int (*debugger_command_hook)(int argc, char **argv);


#ifdef __cplusplus
extern "C" {
#endif

/* interrupts, spinlock, and timers */
extern cpu_status	disable_interrupts(void);
extern void			restore_interrupts(cpu_status status);

extern void			acquire_spinlock(spinlock *lock);
extern void			release_spinlock(spinlock *lock);

extern status_t		install_io_interrupt_handler(long interrupt_number,
						interrupt_handler handler, void *data, ulong flags);
extern status_t		remove_io_interrupt_handler(long interrupt_number,
						interrupt_handler handler, void	*data);

extern status_t		add_timer(timer *t, timer_hook hook, bigtime_t period,
						int32 flags);
extern bool			cancel_timer(timer *t);

/* kernel threads */
extern thread_id	spawn_kernel_thread(thread_func function,
						const char *name, int32 priority, void *arg);
extern status_t		interrupt_thread(thread_id id);

/* signal functions */
extern int			send_signal_etc(pid_t thread, uint signal, uint32 flags);

/* virtual memory */
extern status_t		lock_memory_etc(team_id team, void *buffer, size_t numBytes,
						uint32 flags);
extern status_t		lock_memory(void *buffer, size_t numBytes, uint32 flags);
extern status_t		unlock_memory_etc(team_id team, void *address,
						size_t numBytes, uint32 flags);
extern status_t		unlock_memory(void *buffer, size_t numBytes, uint32 flags);
extern status_t		get_memory_map_etc(team_id team, const void *address,
						size_t numBytes, physical_entry *table,
						uint32* _numEntries);
extern int32		get_memory_map(const void *buffer, size_t size,
						physical_entry *table, int32 numEntries);
extern area_id		map_physical_memory(const char *areaName,
						phys_addr_t physicalAddress, size_t size, uint32 flags,
						uint32 protection, void **_mappedAddress);

/* kernel debugging facilities */
extern void			dprintf(const char *format, ...) _PRINTFLIKE(1, 2);
extern void			kprintf(const char *fmt, ...) _PRINTFLIKE(1, 2);

extern void 		dump_block(const char *buffer, int size, const char *prefix);
						/* TODO: temporary API: hexdumps given buffer */

extern bool			set_dprintf_enabled(bool new_state);

extern void			panic(const char *format, ...) _PRINTFLIKE(1, 2);

extern void			kernel_debugger(const char *message);
extern uint64		parse_expression(const char *string);

extern int			add_debugger_command(const char *name,
						debugger_command_hook hook, const char *help);
extern int			remove_debugger_command(const char *name,
						debugger_command_hook hook);

/* Miscellaneous */
extern void			spin(bigtime_t microseconds);

extern status_t		register_kernel_daemon(daemon_hook hook, void *arg,
						int frequency);
extern status_t		unregister_kernel_daemon(daemon_hook hook, void *arg);

extern void			call_all_cpus(void (*func)(void *, int), void *cookie);
extern void			call_all_cpus_sync(void (*func)(void *, int), void *cookie);
extern void			memory_read_barrier(void);
extern void			memory_write_barrier(void);

/* safe methods to access user memory without having to lock it */
extern status_t		user_memcpy(void *to, const void *from, size_t size);
extern ssize_t		user_strlcpy(char *to, const char *from, size_t size);
extern status_t		user_memset(void *start, char c, size_t count);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_EXPORT_H */
