/* ++++++++++
	KernelExport.h

	Functions exported from the kernel for driver use that are not already
	prototyped elsewhere.
	
	Copyright 1996-98, Be Incorporated.
+++++ */


#ifndef _KERNEL_EXPORT_H
#define _KERNEL_EXPORT_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif


/* ---
	kernel threads
--- */

extern _IMPEXP_KERNEL thread_id spawn_kernel_thread (
	thread_entry	function, 
	const char 		*thread_name, 
	long			priority,
	void			*arg
);


/* ---
	disable/restore interrupt enable flag on current cpu
--- */

typedef ulong		cpu_status;

extern _IMPEXP_KERNEL cpu_status	disable_interrupts();
extern _IMPEXP_KERNEL void			restore_interrupts(cpu_status status);


/* ---
	spinlocks.  Note that acquire/release should be called with
	interrupts disabled.
--- */

typedef vint32	spinlock;

extern _IMPEXP_KERNEL void			acquire_spinlock (spinlock *lock);
extern _IMPEXP_KERNEL void			release_spinlock (spinlock *lock);


/* ---
	An interrupt handler returns one of the following values:
--- */

#define B_UNHANDLED_INTERRUPT		0		/* pass to next handler */
#define B_HANDLED_INTERRUPT			1		/* don't pass on */
#define B_INVOKE_SCHEDULER			2		/* don't pass on; invoke the scheduler */

typedef int32 (*interrupt_handler) (void *data);

/* interrupt handling support for device drivers */

extern _IMPEXP_KERNEL long 	install_io_interrupt_handler (
	long 				interrupt_number, 
	interrupt_handler	handler, 
	void				*data, 
	ulong 				flags
);
extern _IMPEXP_KERNEL long 	remove_io_interrupt_handler (
	long 				interrupt_number,
	interrupt_handler	handler,
	void				*data
);

/* ---
	timer interrupts services
--- */

typedef struct timer timer;
typedef struct qent	qent;
typedef	int32 (*timer_hook)(timer *);

struct qent {
	int64		key;
	qent		*next;
	qent		*prev;
};

struct timer {
	qent			entry;
	uint16			flags;
	uint16			cpu;
	timer_hook		hook;
	bigtime_t		period;
};

status_t	add_timer(timer *t, timer_hook h, bigtime_t, int32 f);
bool		cancel_timer(timer *t);

#define		B_ONE_SHOT_ABSOLUTE_TIMER		1
#define		B_ONE_SHOT_RELATIVE_TIMER		2
#define		B_PERIODIC_TIMER				3

/* ---
	signal functions
--- */

extern _IMPEXP_KERNEL int	send_signal_etc(pid_t thid, uint sig, uint32 flags);


/* ---
	snooze functions
--- */

extern _IMPEXP_KERNEL status_t	snooze_etc(bigtime_t usecs, int timebase, uint32 flags);


/* ---
	virtual memory buffer functions
--- */

#define		B_DMA_IO		0x00000001
#define		B_READ_DEVICE	0x00000002

typedef struct {
	void		*address;				/* address in physical memory */
	ulong		size;					/* size of block */
} physical_entry;

extern _IMPEXP_KERNEL long		lock_memory (
	void		*buf,			/* -> virtual buffer to lock (make resident) */
	ulong		num_bytes,		/* size of virtual buffer */
	ulong		flags
);

extern _IMPEXP_KERNEL long		unlock_memory (
	void		*buf,			/* -> virtual buffer to unlock */
	ulong		num_bytes,		/* size of virtual buffer */
	ulong		flags
);

extern _IMPEXP_KERNEL long		get_memory_map (
	const void		*address,		/* -> virtual buffer to translate */
	ulong			size,			/* size of virtual buffer */
	physical_entry	*table,			/* -> caller supplied table */
	long			num_entries		/* # entries in table */
);
	

/* -----
	address specifications for mapping physical memory
----- */

#define	B_ANY_KERNEL_BLOCK_ADDRESS	((B_ANY_KERNEL_ADDRESS)+1)

/* -----
	MTR attributes for mapping physical memory (Intel Architecture only)
----- */

#define	B_MTR_UC	0x10000000
#define	B_MTR_WC	0x20000000
#define	B_MTR_WT	0x30000000
#define	B_MTR_WP	0x40000000
#define	B_MTR_WB	0x50000000
#define	B_MTR_MASK	0xf0000000

/* -----
	call to map physical memory - typically used for memory-mapped i/o
----- */

extern _IMPEXP_KERNEL area_id	map_physical_memory (
	const char	*area_name,
	void		*physical_address,
	size_t		size,
	uint32		flags,
	uint32		protection,
	void		**mapped_address
);


/* -----
	hardware inquiry
----- */

/* platform_type return value is defined in OS.h */

extern _IMPEXP_KERNEL platform_type	platform();
#if __POWERPC__
extern _IMPEXP_KERNEL long			motherboard_version (void);
extern _IMPEXP_KERNEL long			io_card_version (void);
#endif


/* ---
	primitive kernel debugging facilities.   Debug output is on...

	bebox: serial port 4 
	mac: modem port
	pc: com1
	
	...at 19.2 kbaud, no parity, 8 bit, 1 stop bit.
--- */

#if __GNUC__
extern _IMPEXP_KERNEL void		dprintf (const char *format, ...)		/* just like printf */
                                  __attribute__ ((format (__printf__, 1, 2)));
extern _IMPEXP_KERNEL void		kprintf (const char *fmt, ...)          /* only for debugger cmds */
                                  __attribute__ ((format (__printf__, 1, 2)));
#else
extern _IMPEXP_KERNEL void		dprintf (const char *format, ...);		/* just like printf */
extern _IMPEXP_KERNEL void		kprintf (const char *fmt, ...);         /* only for debugger cmds */
#endif
extern _IMPEXP_KERNEL bool		set_dprintf_enabled (bool new_state);	/* returns old state */

extern _IMPEXP_KERNEL void		panic(const char *format, ...);

extern _IMPEXP_KERNEL void		kernel_debugger (const char *message);	/* enter kernel debugger */
extern _IMPEXP_KERNEL ulong		parse_expression (char *str);           /* util for debugger cmds */

/* special return codes for kernel debugger */
#define  B_KDEBUG_CONT   2
#define  B_KDEBUG_QUIT   3

extern _IMPEXP_KERNEL int		add_debugger_command (char *name,       /* add a cmd to debugger */
									int (*func)(int argc, char **argv), 
									char *help);
extern _IMPEXP_KERNEL int		remove_debugger_command (char *name,       /* remove a cmd from debugger */
									int (*func)(int argc, char **argv)); 

/* -----
	misc
----- */

extern _IMPEXP_KERNEL void			spin (bigtime_t num_microseconds);
extern _IMPEXP_KERNEL int			register_kernel_daemon(void (*func)(void *, int), void *arg, int freq);
extern _IMPEXP_KERNEL int			unregister_kernel_daemon(void (*func)(void *, int), void *arg);
extern _IMPEXP_KERNEL void			call_all_cpus(void (*f)(void*, int), void* cookie);


#ifdef __cplusplus
}
#endif

#endif
