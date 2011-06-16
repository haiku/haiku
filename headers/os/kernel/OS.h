/*
 * Copyright 2004-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OS_H
#define _OS_H

//! Kernel specific structures and functions

#include <pthread.h>
#include <stdarg.h>

#include <SupportDefs.h>
#include <StorageDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

/* System constants */

#define B_OS_NAME_LENGTH	32
#define B_PAGE_SIZE			4096
#define B_INFINITE_TIMEOUT	(9223372036854775807LL)

enum {
	B_TIMEOUT						= 0x8,	/* relative timeout */
	B_RELATIVE_TIMEOUT				= 0x8,	/* fails after a relative timeout
												with B_TIMED_OUT */
	B_ABSOLUTE_TIMEOUT				= 0x10,	/* fails after an absolute timeout
												with B_TIMED_OUT */

	/* experimental Haiku only API */
	B_TIMEOUT_REAL_TIME_BASE		= 0x40,
	B_ABSOLUTE_REAL_TIME_TIMEOUT	= B_ABSOLUTE_TIMEOUT
										| B_TIMEOUT_REAL_TIME_BASE
};


/* Types */

typedef int32 area_id;
typedef int32 port_id;
typedef int32 sem_id;
typedef int32 team_id;
typedef int32 thread_id;


/* Areas */

typedef struct area_info {
	area_id		area;
	char		name[B_OS_NAME_LENGTH];
	size_t		size;
	uint32		lock;
	uint32		protection;
	team_id		team;
	uint32		ram_size;
	uint32		copy_count;
	uint32		in_count;
	uint32		out_count;
	void		*address;
} area_info;

/* area locking */
#define B_NO_LOCK				0
#define B_LAZY_LOCK				1
#define B_FULL_LOCK				2
#define B_CONTIGUOUS			3
#define	B_LOMEM					4	/* B_CONTIGUOUS, < 16 MB physical address */
#define	B_32_BIT_FULL_LOCK		5	/* B_FULL_LOCK, < 4 GB physical addresses */
#define	B_32_BIT_CONTIGUOUS		6	/* B_CONTIGUOUS, < 4 GB physical address */

/* address spec for create_area(), and clone_area() */
#define B_ANY_ADDRESS			0
#define B_EXACT_ADDRESS			1
#define B_BASE_ADDRESS			2
#define B_CLONE_ADDRESS			3
#define	B_ANY_KERNEL_ADDRESS	4

/* area protection */
#define B_READ_AREA				1
#define B_WRITE_AREA			2

extern area_id		create_area(const char *name, void **startAddress,
						uint32 addressSpec, size_t size, uint32 lock,
						uint32 protection);
extern area_id		clone_area(const char *name, void **destAddress,
						uint32 addressSpec, uint32 protection, area_id source);
extern area_id		find_area(const char *name);
extern area_id		area_for(void *address);
extern status_t		delete_area(area_id id);
extern status_t		resize_area(area_id id, size_t newSize);
extern status_t		set_area_protection(area_id id, uint32 newProtection);

/* system private, use macros instead */
extern status_t		_get_area_info(area_id id, area_info *areaInfo, size_t size);
extern status_t		_get_next_area_info(team_id team, int32 *cookie,
						area_info *areaInfo, size_t size);

#define get_area_info(id, areaInfo) \
	_get_area_info((id), (areaInfo),sizeof(*(areaInfo)))
#define get_next_area_info(team, cookie, areaInfo) \
	_get_next_area_info((team), (cookie), (areaInfo), sizeof(*(areaInfo)))


/* Ports */

typedef struct port_info {
	port_id		port;
	team_id		team;
	char		name[B_OS_NAME_LENGTH];
	int32		capacity;		/* queue depth */
	int32		queue_count;	/* # msgs waiting to be read */
	int32		total_count;	/* total # msgs read so far */
} port_info;

extern port_id		create_port(int32 capacity, const char *name);
extern port_id		find_port(const char *name);
extern ssize_t		read_port(port_id port, int32 *code, void *buffer,
						size_t bufferSize);
extern ssize_t		read_port_etc(port_id port, int32 *code, void *buffer,
						size_t bufferSize, uint32 flags, bigtime_t timeout);
extern status_t		write_port(port_id port, int32 code, const void *buffer,
						size_t bufferSize);
extern status_t		write_port_etc(port_id port, int32 code, const void *buffer,
						size_t bufferSize, uint32 flags, bigtime_t timeout);
extern status_t		close_port(port_id port);
extern status_t		delete_port(port_id port);

extern ssize_t		port_buffer_size(port_id port);
extern ssize_t		port_buffer_size_etc(port_id port, uint32 flags,
						bigtime_t timeout);
extern ssize_t		port_count(port_id port);
extern status_t		set_port_owner(port_id port, team_id team);

/* system private, use the macros instead */
extern status_t		_get_port_info(port_id port, port_info *portInfo,
						size_t portInfoSize);
extern status_t		_get_next_port_info(team_id team, int32 *cookie,
						port_info *portInfo, size_t portInfoSize);

#define get_port_info(port, info) \
	_get_port_info((port), (info), sizeof(*(info)))
#define get_next_port_info(team, cookie, info) \
	_get_next_port_info((team), (cookie), (info), sizeof(*(info)))


/* WARNING: The following is Haiku experimental API. It might be removed or
   changed in the future. */

typedef struct port_message_info {
	size_t		size;
	uid_t		sender;
	gid_t		sender_group;
	team_id		sender_team;
} port_message_info;

/* similar to port_buffer_size_etc(), but returns (more) info */
extern status_t		_get_port_message_info_etc(port_id port,
						port_message_info *info, size_t infoSize, uint32 flags,
						bigtime_t timeout);

#define get_port_message_info_etc(port, info, flags, timeout) \
	_get_port_message_info_etc((port), (info), sizeof(*(info)), flags, timeout)


/* Semaphores */

typedef struct sem_info {
	sem_id		sem;
	team_id		team;
	char		name[B_OS_NAME_LENGTH];
	int32		count;
	thread_id	latest_holder;
} sem_info;

/* semaphore flags */
enum {
	B_CAN_INTERRUPT				= 0x01,	/* acquisition of the semaphore can be
										   interrupted (system use only) */
	B_CHECK_PERMISSION			= 0x04,	/* ownership will be checked (system use
										   only) */
	B_KILL_CAN_INTERRUPT		= 0x20,	/* acquisition of the semaphore can be
										   interrupted by SIGKILL[THR], even
										   if not B_CAN_INTERRUPT (system use
										   only) */

	/* release_sem_etc() only flags */
	B_DO_NOT_RESCHEDULE			= 0x02,	/* thread is not rescheduled */
	B_RELEASE_ALL				= 0x08,	/* all waiting threads will be woken up,
										   count will be zeroed */
	B_RELEASE_IF_WAITING_ONLY	= 0x10	/* release count only if there are any
										   threads waiting */
};

extern sem_id		create_sem(int32 count, const char *name);
extern status_t		delete_sem(sem_id id);
extern status_t		acquire_sem(sem_id id);
extern status_t		acquire_sem_etc(sem_id id, int32 count, uint32 flags,
						bigtime_t timeout);
extern status_t		release_sem(sem_id id);
extern status_t		release_sem_etc(sem_id id, int32 count, uint32 flags);
/* TODO: the following two calls are not part of the BeOS API, and might be
   changed or even removed for the final release of Haiku R1 */
extern status_t		switch_sem(sem_id semToBeReleased, sem_id id);
extern status_t		switch_sem_etc(sem_id semToBeReleased, sem_id id,
						int32 count, uint32 flags, bigtime_t timeout);
extern status_t		get_sem_count(sem_id id, int32 *threadCount);
extern status_t		set_sem_owner(sem_id id, team_id team);

/* system private, use the macros instead */
extern status_t		_get_sem_info(sem_id id, struct sem_info *info,
						size_t infoSize);
extern status_t		_get_next_sem_info(team_id team, int32 *cookie,
						struct sem_info *info, size_t infoSize);

#define get_sem_info(sem, info) \
	_get_sem_info((sem), (info), sizeof(*(info)))

#define get_next_sem_info(team, cookie, info) \
	_get_next_sem_info((team), (cookie), (info), sizeof(*(info)))


/* Teams */

typedef struct {
	team_id			team;
	int32			thread_count;
	int32			image_count;
	int32			area_count;
	thread_id		debugger_nub_thread;
	port_id			debugger_nub_port;
	int32			argc;
	char			args[64];
	uid_t			uid;
	gid_t			gid;
} team_info;

#define B_CURRENT_TEAM	0
#define B_SYSTEM_TEAM	1

extern status_t		kill_team(team_id team);
	/* see also: send_signal() */

/* system private, use macros instead */
extern status_t		_get_team_info(team_id id, team_info *info, size_t size);
extern status_t		_get_next_team_info(int32 *cookie, team_info *info,
						size_t size);

#define get_team_info(id, info) \
	_get_team_info((id), (info), sizeof(*(info)))

#define get_next_team_info(cookie, info) \
	_get_next_team_info((cookie), (info), sizeof(*(info)))

/* team usage info */

typedef struct {
	bigtime_t		user_time;
	bigtime_t		kernel_time;
} team_usage_info;

enum {
	/* compatible to sys/resource.h RUSAGE_SELF and RUSAGE_CHILDREN */
	B_TEAM_USAGE_SELF		= 0,
	B_TEAM_USAGE_CHILDREN	= -1
};

/* system private, use macros instead */
extern status_t		_get_team_usage_info(team_id team, int32 who,
						team_usage_info *info, size_t size);

#define get_team_usage_info(team, who, info) \
	_get_team_usage_info((team), (who), (info), sizeof(*(info)))


/* Threads */

typedef enum {
	B_THREAD_RUNNING	= 1,
	B_THREAD_READY,
	B_THREAD_RECEIVING,
	B_THREAD_ASLEEP,
	B_THREAD_SUSPENDED,
	B_THREAD_WAITING
} thread_state;

typedef struct {
	thread_id		thread;
	team_id			team;
	char			name[B_OS_NAME_LENGTH];
	thread_state	state;
	int32			priority;
	sem_id			sem;
	bigtime_t		user_time;
	bigtime_t		kernel_time;
	void			*stack_base;
	void			*stack_end;
} thread_info;

#define B_IDLE_PRIORITY					0
#define B_LOWEST_ACTIVE_PRIORITY		1
#define B_LOW_PRIORITY					5
#define B_NORMAL_PRIORITY				10
#define B_DISPLAY_PRIORITY				15
#define	B_URGENT_DISPLAY_PRIORITY		20
#define	B_REAL_TIME_DISPLAY_PRIORITY	100
#define	B_URGENT_PRIORITY				110
#define B_REAL_TIME_PRIORITY			120

#define B_SYSTEM_TIMEBASE				0
	/* time base for snooze_*(), compatible with the clockid_t constants defined
	   in <time.h> */

#define B_FIRST_REAL_TIME_PRIORITY		B_REAL_TIME_DISPLAY_PRIORITY

typedef status_t (*thread_func)(void *);
#define thread_entry thread_func
	/* thread_entry is for backward compatibility only! Use thread_func */

extern thread_id	spawn_thread(thread_func, const char *name, int32 priority,
						void *data);
extern status_t		kill_thread(thread_id thread);
extern status_t		resume_thread(thread_id thread);
extern status_t		suspend_thread(thread_id thread);

extern status_t		rename_thread(thread_id thread, const char *newName);
extern status_t		set_thread_priority(thread_id thread, int32 newPriority);
extern void			exit_thread(status_t status);
extern status_t		wait_for_thread(thread_id thread, status_t *returnValue);
extern status_t		on_exit_thread(void (*callback)(void *), void *data);

extern thread_id 	find_thread(const char *name);

extern status_t		send_data(thread_id thread, int32 code, const void *buffer,
						size_t bufferSize);
extern int32		receive_data(thread_id *sender, void *buffer,
						size_t bufferSize);
extern bool			has_data(thread_id thread);

extern status_t		snooze(bigtime_t amount);
extern status_t		snooze_etc(bigtime_t amount, int timeBase, uint32 flags);
extern status_t		snooze_until(bigtime_t time, int timeBase);

/* system private, use macros instead */
extern status_t		_get_thread_info(thread_id id, thread_info *info, size_t size);
extern status_t		_get_next_thread_info(team_id team, int32 *cookie,
						thread_info *info, size_t size);

#define get_thread_info(id, info) \
	_get_thread_info((id), (info), sizeof(*(info)))

#define get_next_thread_info(team, cookie, info) \
	_get_next_thread_info((team), (cookie), (info), sizeof(*(info)))

/* bridge to the pthread API */
extern thread_id	get_pthread_thread_id(pthread_t thread);
/* TODO: Would be nice to have, but we use TLS to associate a thread with its
   pthread object. So this is not trivial to implement.
extern status_t		convert_to_pthread(thread_id thread, pthread_t *_thread);
*/


/* Time */

extern uint32		real_time_clock(void);
extern void			set_real_time_clock(uint32 secsSinceJan1st1970);
extern bigtime_t	real_time_clock_usecs(void);
extern bigtime_t	system_time(void);
						/* time since booting in microseconds */
extern nanotime_t	system_time_nsecs();
						/* time since booting in nanoseconds */

					// deprecated (is no-op)
extern status_t		set_timezone(const char *timezone);

/* Alarm */

enum {
	B_ONE_SHOT_ABSOLUTE_ALARM	= 1,
	B_ONE_SHOT_RELATIVE_ALARM,
	B_PERIODIC_ALARM			/* "when" specifies the period */
};

extern bigtime_t	set_alarm(bigtime_t when, uint32 flags);


/* Debugger */

extern void			debugger(const char *message);

/*
   calling this function with a non-zero value will cause your thread
   to receive signals for any exceptional conditions that occur (i.e.
   you'll get SIGSEGV for data access exceptions, SIGFPE for floating
   point errors, SIGILL for illegal instructions, etc).

   to re-enable the default debugger pass a zero.
*/
extern int			disable_debugger(int state);

/* TODO: Remove. Temporary debug helper. */
extern void			debug_printf(const char *format, ...)
						__attribute__ ((format (__printf__, 1, 2)));
extern void			debug_vprintf(const char *format, va_list args);
extern void			ktrace_printf(const char *format, ...)
						__attribute__ ((format (__printf__, 1, 2)));
extern void			ktrace_vprintf(const char *format, va_list args);


/* System information */

#if __INTEL__
#	define B_MAX_CPU_COUNT	8
#elif __x86_64__
#	define B_MAX_CPU_COUNT	8
#elif __POWERPC__
#	define B_MAX_CPU_COUNT	8
#elif __M68K__
#	define B_MAX_CPU_COUNT	1
#elif __ARM__
#	define B_MAX_CPU_COUNT	1
#elif __MIPSEL__
#	define B_MAX_CPU_COUNT	1
#else
#	warning Unknown cpu
#	define B_MAX_CPU_COUNT	1
#endif

typedef enum cpu_types {
	/* TODO: add latest models */

	/* Motorola/IBM */
	B_CPU_PPC_UNKNOWN					= 0,
	B_CPU_PPC_601						= 1,
	B_CPU_PPC_602						= 7,
	B_CPU_PPC_603						= 2,
	B_CPU_PPC_603e						= 3,
	B_CPU_PPC_603ev						= 8,
	B_CPU_PPC_604						= 4,
	B_CPU_PPC_604e						= 5,
	B_CPU_PPC_604ev						= 9,
	B_CPU_PPC_620						= 10,
	B_CPU_PPC_750   					= 6,
	B_CPU_PPC_686						= 13,
	B_CPU_PPC_860						= 25,
	B_CPU_PPC_7400						= 26,
	B_CPU_PPC_7410						= 27,
	B_CPU_PPC_7447A						= 28,
	B_CPU_PPC_7448						= 29,
	B_CPU_PPC_7450						= 30,
	B_CPU_PPC_7455						= 31,
	B_CPU_PPC_7457						= 32,
	B_CPU_PPC_8240						= 33,
	B_CPU_PPC_8245						= 34,

	B_CPU_PPC_IBM_401A1					= 35,
	B_CPU_PPC_IBM_401B2					= 36,
	B_CPU_PPC_IBM_401C2					= 37,
	B_CPU_PPC_IBM_401D2					= 38,
	B_CPU_PPC_IBM_401E2					= 39,
	B_CPU_PPC_IBM_401F2					= 40,
	B_CPU_PPC_IBM_401G2					= 41,
	B_CPU_PPC_IBM_403					= 42,
	B_CPU_PPC_IBM_405GP					= 43,
	B_CPU_PPC_IBM_405L					= 44,
	B_CPU_PPC_IBM_750FX					= 45,
	B_CPU_PPC_IBM_POWER3				= 46,

	/* Intel */

	/* Updated according to Intel(R) Processor Identification and
	 * the  CPUID instruction (Table 4)
	 * AP-485 Intel - 24161832.pdf
	 */
	B_CPU_INTEL_x86						= 0x1000,
	B_CPU_INTEL_PENTIUM					= 0x1051,
	B_CPU_INTEL_PENTIUM75,
	B_CPU_INTEL_PENTIUM_486_OVERDRIVE,
	B_CPU_INTEL_PENTIUM_MMX,
	B_CPU_INTEL_PENTIUM_MMX_MODEL_4		= B_CPU_INTEL_PENTIUM_MMX,
	B_CPU_INTEL_PENTIUM_MMX_MODEL_8		= 0x1058,
	B_CPU_INTEL_PENTIUM75_486_OVERDRIVE,
	B_CPU_INTEL_PENTIUM_PRO				= 0x1061,
	B_CPU_INTEL_PENTIUM_II				= 0x1063,
	B_CPU_INTEL_PENTIUM_II_MODEL_3		= 0x1063,
	B_CPU_INTEL_PENTIUM_II_MODEL_5		= 0x1065,
	B_CPU_INTEL_CELERON					= 0x1066,
	B_CPU_INTEL_CELERON_MODEL_22		= 0x11066,
	B_CPU_INTEL_PENTIUM_III				= 0x1067,
	B_CPU_INTEL_PENTIUM_III_MODEL_8		= 0x1068,
	B_CPU_INTEL_PENTIUM_M				= 0x1069,
	B_CPU_INTEL_PENTIUM_III_XEON		= 0x106a,
	B_CPU_INTEL_PENTIUM_III_MODEL_11 	= 0x106b,
	B_CPU_INTEL_ATOM			= 0x1106c,
	B_CPU_INTEL_PENTIUM_M_MODEL_13		= 0x106d, /* Dothan */
	B_CPU_INTEL_PENTIUM_CORE,
	B_CPU_INTEL_PENTIUM_CORE_2,
	B_CPU_INTEL_PENTIUM_CORE_2_45_NM	= 0x11067, /* Core 2 on 45 nm
	                                                   (Core 2 Extreme,
	                                                   Xeon model 23 or
	                                                   Core 2 Duo/Quad) */
	B_CPU_INTEL_PENTIUM_CORE_I5_M430	= 0x21065, /* Core i5 M 430 @ 2.27 */
	B_CPU_INTEL_PENTIUM_CORE_I7			= 0x1106a, /* Core i7 920 @ 2.6(6) */
	B_CPU_INTEL_PENTIUM_CORE_I7_Q720	= 0x1106e, /* Core i7 Q720 @ 1.6 */
	B_CPU_INTEL_PENTIUM_IV				= 0x10f0,
	B_CPU_INTEL_PENTIUM_IV_MODEL_1,
	B_CPU_INTEL_PENTIUM_IV_MODEL_2,
	B_CPU_INTEL_PENTIUM_IV_MODEL_3,
	B_CPU_INTEL_PENTIUM_IV_MODEL_4,

	/* AMD */

	/* Checked with "AMD Processor Recognition Application Note"
	 * (Table 3)
	 * 20734.pdf
	 */
	B_CPU_AMD_x86						= 0x1100,
	B_CPU_AMD_K5_MODEL_0				= 0x1150,
	B_CPU_AMD_K5_MODEL_1,
	B_CPU_AMD_K5_MODEL_2,
	B_CPU_AMD_K5_MODEL_3,
	B_CPU_AMD_K6_MODEL_6				= 0x1156,
	B_CPU_AMD_K6_MODEL_7				= 0x1157,
	B_CPU_AMD_K6_MODEL_8				= 0x1158,
	B_CPU_AMD_K6_2						= 0x1158,
	B_CPU_AMD_K6_MODEL_9				= 0x1159,
	B_CPU_AMD_K6_III					= 0x1159,
	B_CPU_AMD_K6_III_MODEL_13			= 0x115d,

	B_CPU_AMD_ATHLON_MODEL_1			= 0x1161,
	B_CPU_AMD_ATHLON_MODEL_2			= 0x1162,

	B_CPU_AMD_DURON 					= 0x1163,

	B_CPU_AMD_ATHLON_THUNDERBIRD		= 0x1164,
	B_CPU_AMD_ATHLON_XP 				= 0x1166,
	B_CPU_AMD_ATHLON_XP_MODEL_7,
	B_CPU_AMD_ATHLON_XP_MODEL_8,
	B_CPU_AMD_ATHLON_XP_MODEL_10		= 0x116a, /* Barton */

	B_CPU_AMD_SEMPRON_MODEL_8			= B_CPU_AMD_ATHLON_XP_MODEL_8,
	B_CPU_AMD_SEMPRON_MODEL_10			= B_CPU_AMD_ATHLON_XP_MODEL_10,

	/* According to "Revision Guide for AMD Family 10h
	 * Processors" (41322.pdf)
	 */
	B_CPU_AMD_PHENOM					= 0x11f2,

	/* According to "Revision guide for AMD Athlon 64
	 * and AMD Opteron Processors" (25759.pdf)
	 */
	B_CPU_AMD_ATHLON_64_MODEL_3			= 0x11f3,
	B_CPU_AMD_ATHLON_64_MODEL_4,
	B_CPU_AMD_ATHLON_64_MODEL_5,
	B_CPU_AMD_PHENOM_II					= B_CPU_AMD_ATHLON_64_MODEL_4,
	B_CPU_AMD_OPTERON					= B_CPU_AMD_ATHLON_64_MODEL_5,
	B_CPU_AMD_ATHLON_64_FX				= B_CPU_AMD_ATHLON_64_MODEL_5,
	B_CPU_AMD_ATHLON_64_MODEL_7			= 0x11f7,
	B_CPU_AMD_ATHLON_64_MODEL_8,
	B_CPU_AMD_ATHLON_64_MODEL_11		= 0x11fb,
	B_CPU_AMD_ATHLON_64_MODEL_12,
	B_CPU_AMD_ATHLON_64_MODEL_14		= 0x11fe,
	B_CPU_AMD_ATHLON_64_MODEL_15,

	B_CPU_AMD_GEODE_LX					= 0x115a,

	/* VIA/Cyrix */
	B_CPU_CYRIX_x86						= 0x1200,
	B_CPU_VIA_CYRIX_x86					= 0x1200,
	B_CPU_CYRIX_GXm						= 0x1254,
	B_CPU_CYRIX_6x86MX					= 0x1260,

	/* VIA/IDT */
	B_CPU_IDT_x86						= 0x1300,
	B_CPU_VIA_IDT_x86					= 0x1300,
	B_CPU_IDT_WINCHIP_C6				= 0x1354,
	B_CPU_IDT_WINCHIP_2					= 0x1358,
	B_CPU_IDT_WINCHIP_3,
	B_CPU_VIA_C3_SAMUEL					= 0x1366,
	B_CPU_VIA_C3_SAMUEL_2				= 0x1367,
	B_CPU_VIA_C3_EZRA_T					= 0x1368,
	B_CPU_VIA_C3_NEHEMIAH				= 0x1369,
	B_CPU_VIA_C7_ESTHER					= 0x136a,
	B_CPU_VIA_C7_ESTHER_2				= 0x136d,
	B_CPU_VIA_NANO_ISAIAH				= 0x136f,

	/* Transmeta */
	B_CPU_TRANSMETA_x86					= 0x1600,
	B_CPU_TRANSMETA_CRUSOE				= 0x1654,
	B_CPU_TRANSMETA_EFFICEON			= 0x16f2,
	B_CPU_TRANSMETA_EFFICEON_2			= 0x16f3,

	/* Rise */
	B_CPU_RISE_x86						= 0x1400,
	B_CPU_RISE_mP6						= 0x1450,

	/* National Semiconductor */
	B_CPU_NATIONAL_x86					= 0x1500,
	B_CPU_NATIONAL_GEODE_GX1			= 0x1554,
	B_CPU_NATIONAL_GEODE_GX2,

	/* For compatibility */
	B_CPU_AMD_29K						= 14,
	B_CPU_x86,
	B_CPU_MC6502,
	B_CPU_Z80,
	B_CPU_ALPHA,
	B_CPU_MIPS,
	B_CPU_HPPA,
	B_CPU_M68K,
	B_CPU_ARM,
	B_CPU_SH,
	B_CPU_SPARC
} cpu_type;

#define B_CPU_x86_VENDOR_MASK	0xff00

#ifdef __INTEL__
typedef union {
	struct {
		uint32	max_eax;
		char	vendor_id[12];
	} eax_0;

	struct {
		uint32	stepping		: 4;
		uint32	model			: 4;
		uint32	family			: 4;
		uint32	type			: 2;
		uint32	reserved_0		: 2;
		uint32	extended_model	: 4;
		uint32	extended_family	: 8;
		uint32	reserved_1		: 4;

		uint32	brand_index		: 8;
		uint32	clflush			: 8;
		uint32	logical_cpus	: 8;
		uint32	apic_id			: 8;

		uint32	features;
		uint32	extended_features;
	} eax_1;

	struct {
		uint8	call_num;
		uint8	cache_descriptors[15];
	} eax_2;

	struct {
		uint32	reserved[2];
		uint32	serial_number_high;
		uint32	serial_number_low;
	} eax_3;

	char		as_chars[16];

	struct {
		uint32	eax;
		uint32	ebx;
		uint32	edx;
		uint32	ecx;
	} regs;
} cpuid_info;

extern status_t		get_cpuid(cpuid_info *info, uint32 eaxRegister,
						uint32 cpuNum);
#endif


typedef enum platform_types {
	B_BEBOX_PLATFORM = 0,
	B_MAC_PLATFORM,
	B_AT_CLONE_PLATFORM,
	B_ENIAC_PLATFORM,
	B_APPLE_II_PLATFORM,
	B_CRAY_PLATFORM,
	B_LISA_PLATFORM,
	B_TI_994A_PLATFORM,
	B_TIMEX_SINCLAIR_PLATFORM,
	B_ORAC_1_PLATFORM,
	B_HAL_PLATFORM,
	B_BESM_6_PLATFORM,
	B_MK_61_PLATFORM,
	B_NINTENDO_64_PLATFORM,
	B_AMIGA_PLATFORM,
	B_ATARI_PLATFORM
} platform_type;

typedef struct {
	bigtime_t	active_time;	/* usec of doing useful work since boot */
} cpu_info;


typedef int32 machine_id[2];	/* unique machine ID */

typedef struct {
	machine_id		id;					/* unique machine ID */
	bigtime_t		boot_time;			/* time of boot (usecs since 1/1/1970) */

	int32			cpu_count;			/* number of cpus */
	enum cpu_types	cpu_type;			/* type of cpu */
	int32			cpu_revision;		/* revision # of cpu */
	cpu_info		cpu_infos[B_MAX_CPU_COUNT];	/* info about individual cpus */
	int64			cpu_clock_speed;	/* processor clock speed (Hz) */
	int64			bus_clock_speed;	/* bus clock speed (Hz) */
	enum platform_types platform_type;	/* type of machine we're on */

	int32			max_pages;			/* total # of accessible pages */
	int32			used_pages;			/* # of accessible pages in use */
	int32			page_faults;		/* # of page faults */
	int32			max_sems;
	int32			used_sems;
	int32			max_ports;
	int32			used_ports;
	int32			max_threads;
	int32			used_threads;
	int32			max_teams;
	int32			used_teams;

	char			kernel_name[B_FILE_NAME_LENGTH];
	char			kernel_build_date[B_OS_NAME_LENGTH];
	char			kernel_build_time[B_OS_NAME_LENGTH];
	int64			kernel_version;

	bigtime_t		_busy_wait_time;	/* reserved for whatever */

	int32			cached_pages;
	uint32			abi;				/* the system API */
	int32			ignored_pages;		/* # of ignored/inaccessible pages */
	int32			pad;
} system_info;

/* system private, use macro instead */
extern status_t		_get_system_info(system_info *info, size_t size);

#define get_system_info(info) \
	_get_system_info((info), sizeof(*(info)))

extern int32		is_computer_on(void);
extern double		is_computer_on_fire(void);


/* signal related functions */
int		send_signal(thread_id threadID, unsigned int signal);
void	set_signal_stack(void* base, size_t size);


/* WARNING: Experimental API! */

enum {
	B_OBJECT_TYPE_FD		= 0,
	B_OBJECT_TYPE_SEMAPHORE	= 1,
	B_OBJECT_TYPE_PORT		= 2,
	B_OBJECT_TYPE_THREAD	= 3
};

enum {
	B_EVENT_READ				= 0x0001,	/* FD/port readable */
	B_EVENT_WRITE				= 0x0002,	/* FD/port writable */
	B_EVENT_ERROR				= 0x0004,	/* FD error */
	B_EVENT_PRIORITY_READ		= 0x0008,	/* FD priority readable */
	B_EVENT_PRIORITY_WRITE		= 0x0010,	/* FD priority writable */
	B_EVENT_HIGH_PRIORITY_READ	= 0x0020,	/* FD high priority readable */
	B_EVENT_HIGH_PRIORITY_WRITE	= 0x0040,	/* FD high priority writable */
	B_EVENT_DISCONNECTED		= 0x0080,	/* FD disconnected */

	B_EVENT_ACQUIRE_SEMAPHORE	= 0x0001,	/* semaphore can be acquired */

	B_EVENT_INVALID				= 0x1000	/* FD/port/sem/thread ID not or
											   no longer valid (e.g. has been
											   close/deleted) */
};

typedef struct object_wait_info {
	int32		object;						/* ID of the object */
	uint16		type;						/* type of the object */
	uint16		events;						/* events mask */
} object_wait_info;

/* wait_for_objects[_etc]() waits until at least one of the specified events or,
   if given, the timeout occurred. When entering the function the
   object_wait_info::events field specifies the events for each object the
   caller is interested in. When the function returns the fields reflect the
   events that actually occurred. The events B_EVENT_INVALID, B_EVENT_ERROR,
   and B_EVENT_DISCONNECTED don't need to be specified. They will always be
   reported, when they occur. */

extern ssize_t		wait_for_objects(object_wait_info* infos, int numInfos);
extern ssize_t		wait_for_objects_etc(object_wait_info* infos, int numInfos,
						uint32 flags, bigtime_t timeout);


#ifdef __cplusplus
}
#endif

#endif /* _OS_H */
