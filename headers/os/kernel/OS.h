/* Kernel specific structures and functions
**
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _OS_H
#define _OS_H


#include <SupportDefs.h>
#include <StorageDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------*/
/* System constants */

#define B_OS_NAME_LENGTH	32
#define B_PAGE_SIZE			4096
#define B_INFINITE_TIMEOUT	(9223372036854775807LL)

enum {
	B_TIMEOUT			= 8,	/* relative timeout */
	B_RELATIVE_TIMEOUT	= 8,	/* fails after a relative timeout with B_WOULD_BLOCK */
	B_ABSOLUTE_TIMEOUT	= 16,	/* fails after an absolute timeout with B_WOULD BLOCK */
};

/*-------------------------------------------------------------*/
/* Types */

typedef int32 area_id;
typedef int32 port_id;
typedef int32 sem_id;
typedef int32 team_id;
typedef int32 thread_id;


/*-------------------------------------------------------------*/
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
#define	B_LOMEM					4

/* address spec for create_area(), and clone_area() */
#define B_ANY_ADDRESS			0	
#define B_EXACT_ADDRESS			1	
#define B_BASE_ADDRESS			2	
#define B_CLONE_ADDRESS			3	
#define	B_ANY_KERNEL_ADDRESS	4

/* area protection */
#define B_READ_AREA				1	
#define B_WRITE_AREA			2	

extern area_id	create_area(const char *name, void **start_addr, uint32 addr_spec,
					size_t size, uint32 lock, uint32 protection);
extern area_id	clone_area(const char *name, void **dest_addr, uint32 addr_spec,
					uint32 protection, area_id source);
extern area_id	find_area(const char *name);
extern area_id	area_for(void *address);
extern status_t	delete_area(area_id id);
extern status_t	resize_area(area_id id, size_t new_size);
extern status_t	set_area_protection(area_id id, uint32 new_protection);

/* system private, use macros instead */
extern status_t	_get_area_info(area_id id, area_info *areaInfo, size_t size);
extern status_t	_get_next_area_info(team_id team, int32 *cookie, area_info *areaInfo, size_t size);

#define get_area_info(id, ainfo) \
			_get_area_info((id), (ainfo),sizeof(*(ainfo)))
#define get_next_area_info(team, cookie, ainfo) \
			_get_next_area_info((team), (cookie), (ainfo), sizeof(*(ainfo)))


/*-------------------------------------------------------------*/
/* Ports */

typedef struct port_info {
	port_id		port;
	team_id		team;
	char		name[B_OS_NAME_LENGTH];
	int32		capacity; /* queue depth */
	int32		queue_count; /* # msgs waiting to be read */
	int32		total_count; /* total # msgs read so far */
} port_info;

extern port_id	create_port(int32 capacity, const char *name);
extern port_id	find_port(const char *name);
extern status_t read_port(port_id port, int32 *code, void *buffer, size_t bufferSize);
extern status_t read_port_etc(port_id port, int32 *code, void *buffer, size_t bufferSize,
					uint32 flags, bigtime_t timeout);
extern status_t write_port(port_id port, int32 code, const void *buffer, size_t bufferSize);
extern status_t write_port_etc(port_id port, int32 code, const void *buffer, size_t bufferSize,
					uint32 flags, bigtime_t timeout);
extern status_t close_port(port_id port);
extern status_t delete_port(port_id port);

extern ssize_t	port_buffer_size(port_id port);
extern ssize_t	port_buffer_size_etc(port_id port, uint32 flags, bigtime_t timeout);
extern ssize_t	port_count(port_id port);
extern status_t set_port_owner(port_id port, team_id team);

/* system private, use the macros instead */
extern status_t _get_port_info(port_id port, port_info *portInfo, size_t portInfoSize);
extern status_t _get_next_port_info(team_id team, int32 *cookie, port_info *portInfo,
					size_t portInfoSize);

#define get_port_info(port, info) \
			_get_port_info((port), (info), sizeof(*(info)))
#define get_next_port_info(team, cookie, info) \
			_get_next_port_info((team), (cookie), (info), sizeof(*(info)))


/*-------------------------------------------------------------*/
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
	B_CAN_INTERRUPT		= 1,	/* acquisition of the semaphore can be interrupted (system use only) */
	B_DO_NOT_RESCHEDULE	= 2,	/* thread is not rescheduled after release_sem_etc() */
	B_CHECK_PERMISSION	= 4,	/* ownership will be checked (system use only) */
};

extern sem_id	create_sem(int32 count, const char *name);
extern status_t	delete_sem(sem_id id);
extern status_t	delete_sem_etc(sem_id id, status_t returnCode, bool interrupted); /* ToDo: not public BeOS */
extern status_t	acquire_sem(sem_id id);
extern status_t	acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout);
extern status_t	release_sem(sem_id id);
extern status_t	release_sem_etc(sem_id id, int32 count, uint32 flags);
extern status_t	get_sem_count(sem_id id, int32 *threadCount);
extern status_t	set_sem_owner(sem_id id, team_id team);

/* system private, use the macros instead */
extern status_t	_get_sem_info(sem_id id, struct sem_info *info, size_t infoSize);
extern status_t	_get_next_sem_info(team_id team, int32 *cookie, struct sem_info *info,
					size_t infoSize);

#define get_sem_info(sem, info) \
			_get_sem_info((sem), (info), sizeof(*(info)))

#define get_next_sem_info(team, cookie, info) \
			_get_next_sem_info((team), (cookie), (info), sizeof(*(info)))


/*-------------------------------------------------------------*/
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
#define B_SYSTEM_TEAM	2

extern status_t kill_team(team_id team);
	/* see also: send_signal() */

/* system private, use macros instead */
extern status_t _get_team_info(team_id id, team_info *info, size_t size);
extern status_t _get_next_team_info(int32 *cookie, team_info *info, size_t size);

#define get_team_info(id, info) \
			_get_team_info((id), (info), sizeof(*(info)))

#define get_next_team_info(cookie, info) \
			_get_next_team_info((cookie), (info), sizeof(*(info)))

/* team usage info */

typedef struct {
	bigtime_t		user_time;
	bigtime_t		kernel_time;
} team_usage_info;

/* system private, use macros instead */
extern status_t	_get_team_usage_info(team_id team, int32 who, team_usage_info *tui, size_t size);

#define get_team_usage_info(team, who, info) \
			_get_team_usage_info((team), (who), (info), sizeof(*(info)))

/*-------------------------------------------------------------*/
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

#define B_FIRST_REAL_TIME_PRIORITY		B_REAL_TIME_DISPLAY_PRIORITY
#define B_MIN_PRIORITY					B_IDLE_PRIORITY
#define B_MAX_PRIORITY					B_REAL_TIME_PRIORITY

#define B_SYSTEM_TIMEBASE				0

typedef int32 (*thread_func) (void *);
#define thread_entry thread_func /* thread_entry is for backward compatibility only! Use thread_func */

extern thread_id	spawn_thread(thread_func, const char *name, int32 priority, void *data);
extern status_t		kill_thread(thread_id thread);
extern status_t		resume_thread(thread_id thread);
extern status_t		suspend_thread(thread_id thread);

extern status_t		rename_thread(thread_id thread, const char *newName);
extern status_t		set_thread_priority (thread_id thread, int32 newPriority);
extern void			exit_thread(status_t status);
extern status_t		wait_for_thread (thread_id thread, status_t *threadReturnValue);
extern status_t		on_exit_thread(void (*callback)(void *), void *data);

#if __INTEL__ && !_KERNEL_MODE && !_NO_INLINE_ASM
static inline thread_id
find_thread(const char *name) {
	extern thread_id _kfind_thread_(const char *name);
	if (!name) {
		thread_id thread;
		__asm__ __volatile__ ( 
			"movl	%%fs:4, %%eax \n\t"
			: "=a"(thread) );
		return thread;
	}
	return _kfind_thread_(name);
}
#else
extern thread_id 	find_thread(const char *name);
#endif

extern status_t		send_data(thread_id thread, int32 code, const void *buffer, size_t buffer_size);
extern status_t		receive_data(thread_id *sender, void *buffer, size_t buffer_size);
extern bool			has_data(thread_id thread);

extern status_t		snooze(bigtime_t amount);
extern status_t		snooze_until(bigtime_t time, int timeBase);

/* system private, use macros instead */
extern status_t		_get_thread_info(thread_id id, thread_info *info, size_t size);
extern status_t		_get_next_thread_info(team_id team, int32 *cookie, thread_info *info, size_t size);

#define get_thread_info(id, info) \
			_get_thread_info((id), (info), sizeof(*(info)))

#define get_next_thread_info(team, cookie, info) \
			_get_next_thread_info((team), (cookie), (info), sizeof(*(info)))


/*-------------------------------------------------------------*/
/* Time */

extern uint32		real_time_clock(void);
extern void			set_real_time_clock(uint32 secs_since_jan1_1970);
extern bigtime_t	real_time_clock_usecs(void);
extern status_t		set_timezone(char *timezone);
extern bigtime_t	system_time(void);     /* time since booting in microseconds */


/*-------------------------------------------------------------*/
/* Alarm */

enum {
	B_ONE_SHOT_ABSOLUTE_ALARM	= 1,
	B_ONE_SHOT_RELATIVE_ALARM,
	B_PERIODIC_ALARM			/* "when" specifies the period */
};

extern bigtime_t set_alarm(bigtime_t when, uint32 flags);


/*-------------------------------------------------------------*/
/* Debugger */

extern void	debugger(const char *message);

/*
   calling this function with a non-zero value will cause your thread
   to receive signals for any exceptional conditions that occur (i.e.
   you'll get SIGSEGV for data access exceptions, SIGFPE for floating
   point errors, SIGILL for illegal instructions, etc).

   to re-enable the default debugger pass a zero.
*/   
extern const int disable_debugger(int state);


/*-------------------------------------------------------------*/
/* System information */

#if __INTEL__
#	define B_MAX_CPU_COUNT	8
#elif __POWERPC__
#	define B_MAX_CPU_COUNT	8
#endif

#define OBOS_CPU_TYPES

typedef enum cpu_types {
	// ToDo: add latest models

	/* Motorola/IBM */
	B_CPU_PPC_601						= 1,
	B_CPU_PPC_603						= 2,
	B_CPU_PPC_603e						= 3,
	B_CPU_PPC_604						= 4,
	B_CPU_PPC_604e						= 5,
	B_CPU_PPC_750   					= 6,
	B_CPU_PPC_686						= 13,

	/* Intel */
	B_CPU_INTEL_X86						= 0x1000,
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
	B_CPU_INTEL_PENTIUM_III				= 0x1067,
	B_CPU_INTEL_PENTIUM_III_MODEL_8		= 0x1068,
#ifdef OBOS_CPU_TYPES
	B_CPU_INTEL_PENTIUM_III_MODEL_11 	= 0x106b,
	B_CPU_INTEL_PENTIUM_IV				= 0x10f0,
	B_CPU_INTEL_PENTIUM_IV_MODEL1,
	B_CPU_INTEL_PENTIUM_IV_MODEL2,
	B_CPU_INTEL_PENTIUM_IV_XEON 		= 0x0F27,
#endif

	/* AMD */
	B_CPU_AMD_X86						= 0x1100,
	B_CPU_AMD_K5_MODEL0					= 0x1150,
	B_CPU_AMD_K5_MODEL1,
	B_CPU_AMD_K5_MODEL2,
	B_CPU_AMD_K5_MODEL3,
	B_CPU_AMD_K6_MODEL6					= 0x1156,
	B_CPU_AMD_K6_MODEL7					= 0x1157,
	B_CPU_AMD_K6_MODEL8					= 0x1158,
	B_CPU_AMD_K6_2						= 0x1158,
	B_CPU_AMD_K6_MODEL9					= 0x1159,
	B_CPU_AMD_K6_III					= 0x1159,
#ifdef OBOS_CPU_TYPES
	B_CPU_AMD_K6_III_MODEL2				= 0x115D,
#endif

	B_CPU_AMD_ATHLON_MODEL1				= 0x1161,
#ifdef OBOS_CPU_TYPES
	B_CPU_AMD_ATHLON_MODEL2 			= 0x1162,
		
	B_CPU_AMD_DURON 					= 0x1163,	
	
	B_CPU_AMD_ATHLON_THUNDERBIRD		= 0x1164,
	B_CPU_AMD_ATHLON_XP 				= 0x1166,
	B_CPU_AMD_ATHLON_XP_MODEL2,
	B_CPU_AMD_ATHLON_XP_MODEL3,
	B_CPU_AMD_ATHLON_XP_MODEL_BARTON 	= 0x116A,
#endif
	
	/* VIA */
	B_CPU_CYRIX_X86						= 0x1200,
	B_CPU_CYRIX_GXm						= 0x1254,
	B_CPU_CYRIX_6x86MX					= 0x1260,

	/* others */
	B_CPU_IDT_X86						= 0x1300,
	B_CPU_IDT_WINCHIP_C6				= 0x1354,
	B_CPU_IDT_WINCHIP_2					= 0x1358,

	B_CPU_RISE_X86						= 0x1400,
	B_CPU_RISE_mP6						= 0x1450,

#ifdef OBOS_CPU_TYPES
	B_CPU_NATIONAL_X86					= 0x1500,
	B_CPU_NATIONAL_GEODE_GX1			= 0x1554,
#endif

	/* For compatibility */
	B_CPU_AMD_29K						= 14,
	B_CPU_X86,
	B_CPU_MC6502,
	B_CPU_Z80,
	B_CPU_ALPHA,
	B_CPU_MIPS,
	B_CPU_HPPA,
	B_CPU_M68K,
	B_CPU_ARM,
	B_CPU_SH,
	B_CPU_SPARC,
} cpu_type;

#define B_CPU_X86_VENDOR_MASK	0x1F00

#ifdef __INTEL__
typedef union {
	struct {
		uint32	max_eax;
		char	vendorid[12];
	} eax_0;

	struct {
		uint32	stepping	: 4;
		uint32	model		: 4;
		uint32	family		: 4;
		uint32	type		: 2;
		uint32	reserved_0	: 18;
		
		uint32	reserved_1;
		uint32	features; 
		uint32	reserved_2;
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

extern status_t get_cpuid(cpuid_info* info, uint32 eax_register, uint32 cpu_num);
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
	B_NINTENDO_64_PLATFORM
} platform_type;

typedef struct {
	bigtime_t	active_time;	/* usec of doing useful work since boot */
} cpu_info;


typedef int32 machine_id[2];	/* unique machine ID */

typedef struct {
	machine_id		id;							/* unique machine ID */
	bigtime_t		boot_time;					/* time of boot (usecs since 1/1/1970) */

	int32			cpu_count;					/* number of cpus */
	enum cpu_types	cpu_type;					/* type of cpu */
	int32			cpu_revision;				/* revision # of cpu */
	cpu_info		cpu_infos[B_MAX_CPU_COUNT];	/* info about individual cpus */
	int64			cpu_clock_speed;	 		/* processor clock speed (Hz) */
	int64			bus_clock_speed;			/* bus clock speed (Hz) */
	enum platform_types platform_type;          /* type of machine we're on */

	int32			max_pages;					/* total # physical pages */
	int32			used_pages;					/* # physical pages in use */
	int32			page_faults;				/* # of page faults */
	int32			max_sems;
	int32			used_sems;
	int32			max_ports;
	int32			used_ports;
	int32			max_threads;
	int32			used_threads;
	int32			max_teams;
	int32			used_teams;

//	ToDo: B_FILE_NAME_LENGTH is currently not defined at this point
//	char			kernel_name[B_FILE_NAME_LENGTH];		/* name of kernel */
	char			kernel_name[256];
	char			kernel_build_date[B_OS_NAME_LENGTH];	/* date kernel built */
	char			kernel_build_time[B_OS_NAME_LENGTH];	/* time kernel built */
	int64			kernel_version;             /* version of this kernel */

	bigtime_t		_busy_wait_time;			/* reserved for Be */
	int32			pad[4];   	               	/* just in case... */
} system_info;

/* system private, use macro instead */
extern status_t _get_system_info (system_info *returned_info, size_t size);

#define get_system_info(info) \
			_get_system_info((info), sizeof(*(info)))

extern int32	is_computer_on(void);
extern double	is_computer_on_fire(void);

#ifdef __cplusplus
}
#endif

#endif /* _OS_H */
