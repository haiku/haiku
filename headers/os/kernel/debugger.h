/*******************************************************************************
/
/	File:		debugger.h
/
/	Description:	kernel interface for a debugger.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <BeBuild.h>
#include <OS.h>
#include <image.h>
#include <sys/types.h>
#include <be_prim.h>
#if __GNUC__
#include <signal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -----
	kernel calls
----- */

extern status_t	install_default_debugger (port_id to_debugger_port);
extern port_id	install_team_debugger (team_id team, port_id to_debugger_port);
extern status_t	remove_team_debugger (team_id team);
extern status_t	debug_thread (thread_id thread);


/* -----
	per-thread debugging flags (returned by the get_thread_debug_info
	request to the debugging nub)
----- */

#define	B_STOP_CHILD_THREADS	0x01

/* -----
	ids for why a thread is invoking the debugger
----- */

#if __POWERPC__
typedef enum {
	B_THREAD_NOT_RUNNING,
	B_DEBUGGER_CALL,
	B_BREAKPOINT_HIT,
	B_NMI,
	B_MACHINE_CHECK_EXCEPTION,
	B_DATA_ACCESS_EXCEPTION,
	B_INSTRUCTION_ACCESS_EXCEPTION,
	B_ALIGNMENT_EXCEPTION,
	B_PROGRAM_EXCEPTION,
	B_GET_PROFILING_INFO,
	B_WATCHPOINT_HIT,
	B_SYSCALL_HIT
} db_why_stopped;
#endif

#if __INTEL__
typedef enum {
	B_THREAD_NOT_RUNNING,
	B_DEBUGGER_CALL,
	B_BREAKPOINT_HIT,
	B_SNGLSTP,
	B_NMI,
	B_MACHINE_CHECK_EXCEPTION,
	B_SEGMENT_VIOLATION,
	B_ALIGNMENT_EXCEPTION,
	B_DIVIDE_ERROR,
	B_OVERFLOW_EXCEPTION,
	B_BOUNDS_CHECK_EXCEPTION,
	B_INVALID_OPCODE_EXCEPTION,
	B_SEGMENT_NOT_PRESENT,
	B_STACK_FAULT,
	B_GENERAL_PROTECTION_FAULT,
	B_FLOATING_POINT_EXCEPTION,
	B_GET_PROFILING_INFO,
	B_WATCHPOINT_HIT,
	B_SYSCALL_HIT
} db_why_stopped;
#endif

/* -----
	cpu state.  It is arranged to be useable by the kernel, hence all the
	C volatile regs are grouped at the beginning.  The non-volatile ones
	are only saved when neccessary.
----- */

#if __POWERPC__
typedef struct {
	int32	filler1;
	int32	fpscr;
	int32	pc;
	int32	msr;
	int32	lr;
	int32	ctr;
	int32	xer;
	int32	cr;
	int32	sprg0;
	int32	filler2;	/* force alignment on quad-word */
	int32	filler3;
	int32	filler4;
	int32	r0;
	int32	r1;			/* stack ptr */
	int32	r2;
	int32	r3;
	int32	r4;
	int32	r5;
	int32	r6;
	int32	r7;
	int32	r8;
	int32	r9;
	int32	r10;
	int32	r11;
	int32	r12;
	int32	r13;
	int32	r14;
	int32	r15;
	int32	r16;
	int32	r17;
	int32	r18;
	int32	r19;
	int32	r20;
	int32	r21;
	int32	r22;
	int32	r23;
	int32	r24;
	int32	r25;
	int32	r26;
	int32	r27;
	int32	r28;
	int32	r29;
	int32	r30;
	int32	r31;
	double	f0;
	double	f1;
	double	f2;
	double	f3;
	double	f4;
	double	f5;
	double	f6;
	double	f7;
	double	f8;
	double	f9;
	double	f10;
	double	f11;
	double	f12;
	double	f13;	/* C non-volatile regs start here */
	double	f14;
	double	f15;
	double	f16;
	double	f17;
	double	f18;
	double	f19;
	double	f20;
	double	f21;
	double	f22;
	double	f23;
	double	f24;
	double	f25;
	double	f26;
	double	f27;
	double	f28;
	double	f29;
	double	f30;
	double	f31;
} cpu_state;
#endif

	
/*
 * all the 486 registers including the segment registers and the 
 * general registers.
 */

typedef struct {
#if __INTEL__
	extended_regs	xregs;			/* fpu/mmx/xmm registers */
#else
	char xregs[516];			/* Use placeholder array of right size for now */
#endif
	uint16	gs;
	uint16  reserved0;
	uint16	fs;
	uint16  reserved1;
	uint16	es;
	uint16  reserved2;
	uint16	ds;
	uint16  reserved3;
	ulong	edi;
	ulong	esi;
	ulong	ebp;
	ulong	esp_res;
	ulong	ebx;
	ulong	edx;
	ulong	ecx;
	ulong	eax;
	ulong	trap_no;		/* trap or int number */
	ulong	error_code;		/* trap error code */
	ulong	eip;			/* user eip */
	uint16	cs;				/* user cs */
	uint16  reserved4;
	ulong	eflags;			/* user elfags */
	ulong	uesp;			/* user esp */
	uint16	ss;				/* user ss */
	uint16  reserved5;
} x86_cpu_state;

#if __INTEL__
typedef x86_cpu_state cpu_state;
#endif

/* -----
	messages from debug server to the nub running in a target
	thread's address space.
----- */

enum debug_nub_message {
	B_READ_MEMORY = 0,			/* read some memory */
	B_WRITE_MEMORY,				/* write some memory */
	B_RUN_THREAD,				/* run thread full speed */
	B_STEP_THREAD,				/* step thread while pc in range */
	B_STEP_OVER_THREAD,			/* step thread while pc in range, skip calls */
	B_STEP_OUT_THREAD,			/* step thread till exit current proc */
	B_SET_BREAKPOINT,			/* set a breakpoint */
	B_CLEAR_BREAKPOINT,			/* set a breakpoint */
	B_STOP_NEW_THREADS,			/* en/disable stopping of child threads */
	B_GET_THREAD_DEBUG_INFO,	/* get debugging info */
	B_ACKNOWLEGE_IMAGE_CREATED,	/* acknowlege image created */
	B_START_PROFILER,			/* start profiler */
	B_STOP_PROFILER,			/* stop profiler */
	B_SET_WATCHPOINT,			/* set a watchpoint */
	B_CLEAR_WATCHPOINT,			/* clear a watchpoint */
	B_STOP_ON_DEBUG,            /* stop all threads in team when one enters db*/
	B_GET_THREAD_STACK_TOP,     /* get top of ustack of a thread in the kernel*/
	B_HANDOFF_TO_OTHER_DEBUGGER,/* prepare debug nub for handing off to another debugger */
	B_GET_WHY_STOPPED			/* ask why the thread is stopped */
};

/* -----
	structures passed to the nub
----- */

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	int32		count;					/* # bytes */
	char		*addr;					/* address to read */
} nub_read_memory_msg;
	
typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	int32		count;					/* # bytes */
	char		*addr;					/* address to write */
} nub_write_memory_msg;

typedef struct {
	thread_id	thread;					/* thread id */
	int32		align_to_double;		/* for alignment */
	cpu_state	cpu;					/* cpu state */
} nub_run_thread_msg;

typedef struct {
	thread_id	thread;					/* thread id */
	int32		align_to_double;		/* for alignment */
	cpu_state	cpu;					/* cpu state */
	char		*low_pc;				/* low end of pc range */
	char		*high_pc;				/* highend of pc range */
} nub_step_thread_msg;

typedef struct {
	thread_id	thread;					/* thread id */
	int32		align_to_double;		/* for alignment */
	cpu_state	cpu;					/* cpu state */
	char		*low_pc;				/* low end of pc range */
	char		*high_pc;				/* highend of pc range */
} nub_step_over_thread_msg;

typedef struct {
	thread_id	thread;					/* thread id */
	int32		align_to_double;		/* for alignment */
	cpu_state	cpu;					/* cpu state */
} nub_step_out_thread_msg;

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	char		*addr;					/* breakpoint address */
} nub_set_breakpoint_msg;

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	char		*addr;					/* breakpoint address */
} nub_clear_breakpoint_msg;

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	thread_id	thread;
	bool		enabled;				/* en/disable stop of child threads */
} nub_stop_new_threads_msg;

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	thread_id	thread;
} nub_get_thread_debug_info_msg;

typedef struct {
	int32	debug_flags;				/* returned thread debugging flags */
} nub_get_thread_debug_info_reply;

typedef struct {
	int32	token;
} nub_acknowlege_image_created_msg;



typedef	enum
{
	B_PERFMON_USER_MODE	= 0,
	B_PERFMON_KERNEL_MODE,
	B_PERFMON_KERNEL_AND_USER_MODE
} perfmon_privilege;


typedef	struct
{
	uint32				event;
	uint32				event_qualifier;
	perfmon_privilege	privilege;
	int32				init_val;	/* usually should be 0; should be negative for interrupts */
	uint32				flags;		/* HW-specific bits, usually 0 */
}	perfmon_event_selector;


typedef struct {
	port_id					reply_port;				/* port for reply from kernel */
	thread_id				thid;
	int32					perfmon_counter;
	perfmon_event_selector	perfmon_event;	
	int32					num;
	int32					slots[1];
} nub_start_profiler_msg;	

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	thread_id	thid;
	int32		perfmon_counter;
} nub_stop_profiler_msg;	

typedef struct {
	int32		num;
	int32		slots[1];
} nub_stop_profiler_reply;

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	char		*addr;					/* watchpoint address */
	int			type;					/* watchpoint type */
} nub_set_watchpoint_msg;

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	char		*addr;					/* watchpoint address */
} nub_clear_watchpoint_msg;

typedef struct {
	port_id		reply_port;				/* port for reply from kernel */
	thread_id	thread;                 /* thid of thread to set this for */
	bool		enabled;
} nub_stop_on_debug_msg;

typedef struct {
	port_id		reply_port;
	thread_id	thread;
} nub_get_thread_stack_top_msg;

typedef struct {
	void		*stack_top; 			/* stack ptr at entry to the kernel */
	void        *pc;                	/* program ctr at entry to the kernel */
} nub_get_thread_stack_top_reply;

typedef struct {
	port_id		reply_port;
	port_id		new_db_port;
} nub_handoff_msg;

typedef struct {
	thread_id	thread;
} nub_get_why_stopped_msg;

/* -----
	union of all stuctures passed to the nub
----- */

typedef union {
	nub_read_memory_msg					nub_read_memory;
	nub_write_memory_msg				nub_write_memory;
	nub_run_thread_msg					nub_run_thread;
	nub_step_thread_msg					nub_step_thread;
	nub_step_over_thread_msg			nub_step_over_thread;
	nub_step_out_thread_msg				nub_step_out_thread;
	nub_set_breakpoint_msg				nub_set_breakpoint;
	nub_clear_breakpoint_msg			nub_clear_breakpoint;
	nub_stop_new_threads_msg			nub_stop_new_threads;
	nub_get_thread_debug_info_msg		nub_get_thread_debug_info;
	nub_acknowlege_image_created_msg	nub_acknowlege_image_created;
	nub_start_profiler_msg				nub_start_profiler;
	nub_stop_profiler_msg				nub_stop_profiler;
	nub_set_watchpoint_msg				nub_set_watchpoint;
	nub_clear_watchpoint_msg			nub_clear_watchpoint;
	nub_stop_on_debug_msg				nub_stop_on_debug;
	nub_get_thread_stack_top_msg		nub_get_thread_stack_top;
	nub_handoff_msg						nub_handoff;
	nub_get_why_stopped_msg				nub_get_why_stopped;
} to_nub_msg;


/* -----
	messages passed to the external debugger
----- */

enum debugger_message {
	B_THREAD_STOPPED 	= 0,	/* thread stopped, here is its state */
	B_TEAM_CREATED		= 1,	/* team was created */
	B_TEAM_DELETED		= 2,	/* team was deleted */
	B_IMAGE_CREATED		= 3,	/* image was created */
	B_IMAGE_DELETED		= 4,	/* image was deleted */
	B_THREAD_CREATED	= 5,	/* thread was created */
	B_THREAD_DELETED	= 6,	/* thread was deleted */
	B_SYSCALL_PRE		= 7, 	/* begin of syscall */
	B_SYSCALL_POST		= 8, 	/* end of syscall */
};

/* ----------
	structures passed to the external debugger
----- */

typedef struct {
	thread_id		thread;		/* thread id */
	team_id			team;		/* team id */
	db_why_stopped	why;		/* reason for contacting debugger */
	port_id			nub_port;	/* port to nub for this team */
	cpu_state		cpu;		/* cpu state */
	void			*data;		/* additional data */
} db_thread_stopped_msg;
	
typedef struct {
	team_id		team;			/* id of new team  */
} db_team_created_msg;

typedef struct {
	team_id		team;			/* id of deleted team  */
} db_team_deleted_msg;

typedef struct {
	int32		reply_token;	/* token to acknowledge receipt (REQUIRED!) */
	team_id		team;			/* team id */
	thread_id	thread;			/* id of thread that is loading the image */
	image_info	info;			/* info for the image */
	port_id		nub_port;		/* port to nub for this team */
} db_image_created_msg;

typedef struct {
	team_id		team;
	image_info	info;			/* info for the image */
} db_image_deleted_msg;

typedef struct {
	thread_id	thread;			/* thread id */
	team_id		team;			/* team id */
} db_thread_created_msg;

typedef struct {
	thread_id	thread;			/* thread id */
	team_id		team;			/* team id */
} db_thread_deleted_msg;

typedef struct {
	thread_id	thread;
} db_get_profile_info_msg;

typedef struct {
	thread_id	thread;			/* thread id */
	team_id		team;			/* team id */
	bigtime_t	start_time;		/* time of syscall start */
	uint32		syscall;		/* the syscall number */
	uint32		args[16];		/* syscall args */
} db_syscall_pre_msg;

typedef struct {
	thread_id	thread;			/* thread id */
	team_id		team;			/* team id */
	bigtime_t	start_time;		/* time of syscall start */
	bigtime_t	end_time;		/* time of syscall completion */
	uint32		rethi;			/* upper word of return value */
	uint32		retlo;			/* lower word of return value */
	uint32		syscall;		/* the syscall number */
	uint32		args[16];		/* syscall args */
} db_syscall_post_msg;

/* -----
	union of all structures passed to external debugger
----- */

typedef union {
	db_thread_stopped_msg		thread_stopped;
	db_team_created_msg			team_created;
	db_team_deleted_msg			team_deleted;
	db_image_created_msg		image_created;
	db_image_deleted_msg		image_deleted;
	db_thread_created_msg		thread_created;
	db_thread_deleted_msg		thread_deleted;
	db_get_profile_info_msg		get_profile_info;
	db_syscall_post_msg			syscall_post;
} to_debugger_msg;

/*
 * debugger flags, state constants.
 * Bottom sixteen bits of a word are reserved for Kernel use.
 * Rest are used for user-level control by debuggers
 * using the debugging API. See nukernel/inc/thread.h.
 */
#define B_DEBUG_USER_FLAGS_MASK					0xffff0000

#define B_DEBUG_SYSCALL_TRACING_ONLY			0x00010000
#define B_DEBUG_SYSCALL_FAST_TRACE				0x00020000
#define B_DEBUG_SYSCALL_TRACE_THROUGH_SPAWNS	0x00040000
#define B_DEBUG_SYSCALL_TRACE_WHOLE_TEAM		0x00080000
#define B_DEBUG_SYSCALL_TRACE_PRE				0x00100000
#define B_DEBUG_SYSCALL_TRACE_POST				0x00200000

#ifdef __cplusplus
}
#endif

#endif
