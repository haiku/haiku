#ifndef _LIBROOT_TYPES_H
#define _LIBROOT_TYPES_H

#ifdef __cplusplus
 "C" {
#endif
#include <types.h>
#include <ktypes.h>
#include <defines.h>

#define B_NO_LOCK			0	
#define B_LAZY_LOCK			1	
#define B_FULL_LOCK			2	
#define B_CONTIGUOUS		3	
#define	B_LOMEM				4

#define B_ANY_ADDRESS				0	
#define B_EXACT_ADDRESS				1	
#define B_BASE_ADDRESS				2	
#define B_CLONE_ADDRESS				3	
#define	B_ANY_KERNEL_ADDRESS		4
				
#define B_READ_AREA			1	
#define B_WRITE_AREA		2	

enum {
	B_ONE_SHOT_ABSOLUTE_ALARM = 1,	/* alarm is one-shot and time is specified absolutely */
	B_ONE_SHOT_RELATIVE_ALARM = 2,	/* alarm is one-shot and time is specified relatively */
	B_PERIODIC_ALARM = 3			/* alarm is periodic and time is the period */
};

#define B_LOW_PRIORITY						5
#define B_NORMAL_PRIORITY					10
#define B_DISPLAY_PRIORITY					15
#define	B_URGENT_DISPLAY_PRIORITY			20
#define	B_REAL_TIME_DISPLAY_PRIORITY		100
#define	B_URGENT_PRIORITY					110
#define B_REAL_TIME_PRIORITY				120

  thread_id spawn_thread (
	thread_func		function_name, 
	const char 		*thread_name, 
	int32			priority, 
	void			*arg
);
				 
#define B_SYSTEM_TIMEBASE  (0)
#define	B_SYSTEM_TEAM		2

typedef struct {
	team_id			team;
	int32			image_count;
	int32			thread_count;
	int32			area_count;
	thread_id		debugger_nub_thread;
	port_id			debugger_nub_port;

	int32           argc;      /* number of args on the command line */
	char            args[64];  /* abbreviated command line args */
	uid_t        	uid;
	gid_t        	gid;
} team_info;

#if __INTEL__
#define		B_MAX_CPU_COUNT		8
#endif

#if __POWERPC__
#define		B_MAX_CPU_COUNT		8
#endif

typedef enum cpu_types {
	B_CPU_PPC_601	= 1,
	B_CPU_PPC_603	= 2,
	B_CPU_PPC_603e	= 3,
	B_CPU_PPC_604	= 4,
	B_CPU_PPC_604e	= 5,
	B_CPU_PPC_750   = 6,
	B_CPU_PPC_686	= 13,
	B_CPU_X86,
	B_CPU_ALPHA,
	B_CPU_MIPS,
	B_CPU_HPPA,
	B_CPU_M68K,
	B_CPU_ARM,
	B_CPU_SH,
	B_CPU_SPARC,
// Since these have specific values, AFAIK, they will stay, even unsupported
	B_CPU_INTEL_X86 = 0x1000,
	B_CPU_INTEL_PENTIUM = 0x1051,
	B_CPU_INTEL_PENTIUM75,
	B_CPU_INTEL_PENTIUM_486_OVERDRIVE,
	B_CPU_INTEL_PENTIUM_MMX,
	B_CPU_INTEL_PENTIUM_MMX_MODEL_4 = B_CPU_INTEL_PENTIUM_MMX,
	B_CPU_INTEL_PENTIUM_MMX_MODEL_8 = 0x1058,
	B_CPU_INTEL_PENTIUM75_486_OVERDRIVE,
	B_CPU_INTEL_PENTIUM_PRO = 0x1061,
	B_CPU_INTEL_PENTIUM_II = 0x1063,
	B_CPU_INTEL_PENTIUM_II_MODEL_3 = 0x1063,
	B_CPU_INTEL_PENTIUM_II_MODEL_5 = 0x1065,
	B_CPU_INTEL_CELERON = 0x1066,
	B_CPU_INTEL_PENTIUM_III = 0x1067,
	B_CPU_INTEL_PENTIUM_III_MODEL_8 = 0x1068,
	
	B_CPU_AMD_X86 = 0x1100,
	B_CPU_AMD_K5_MODEL0 = 0x1150,
	B_CPU_AMD_K5_MODEL1,
	B_CPU_AMD_K5_MODEL2,
	B_CPU_AMD_K5_MODEL3,

	B_CPU_AMD_K6_MODEL6 = 0x1156,
	B_CPU_AMD_K6_MODEL7 = 0x1157,

	B_CPU_AMD_K6_MODEL8 = 0x1158,
	B_CPU_AMD_K6_2 = 0x1158,

	B_CPU_AMD_K6_MODEL9 = 0x1159,
	B_CPU_AMD_K6_III = 0x1159,
	
	B_CPU_AMD_ATHLON_MODEL1 = 0x1161,

	B_CPU_CYRIX_X86 = 0x1200,
	B_CPU_CYRIX_GXm = 0x1254,
	B_CPU_CYRIX_6x86MX = 0x1260,

	B_CPU_IDT_X86 = 0x1300,
	B_CPU_IDT_WINCHIP_C6 = 0x1354,
	B_CPU_IDT_WINCHIP_2 = 0x1358,
	
	B_CPU_RISE_X86 = 0x1400,
	B_CPU_RISE_mP6 = 0x1450

} cpu_type;

#define B_CPU_X86_VENDOR_MASK	0x1F00

typedef enum platform_types {
	B_BEBOX_PLATFORM = 0,
	B_MAC_PLATFORM,
	B_AT_CLONE_PLATFORM
} platform_type;

typedef struct {
	bigtime_t		active_time;		/* # usec doing useful work since boot */
} cpu_info;


typedef int32 machine_id[2];		/* unique machine ID */

typedef struct {
	machine_id	   id;							/* unique machine ID */
	bigtime_t	   boot_time;					/* time of boot (# usec since 1/1/70) */

	int32		   cpu_count;					/* # of cpus */
	enum cpu_types cpu_type;					/* type of cpu */
	int32		   cpu_revision;				/* revision # of cpu */
	cpu_info	   cpu_infos[B_MAX_CPU_COUNT];	/* info about individual cpus */
	int64          cpu_clock_speed;	 			/* processor clock speed (Hz) */
	int64          bus_clock_speed;				/* bus clock speed (Hz) */
	enum platform_types platform_type;          /* type of machine we're on */

	int32		  max_pages;					/* total # physical pages */
	int32		  used_pages;					/* # physical pages in use */
	int32		  page_faults;					/* # of page faults */
	int32		  max_sems;						/* maximum # semaphores */
	int32		  used_sems;					/* # semaphores in use */
	int32		  max_ports;					/* maximum # ports */
	int32		  used_ports;					/* # ports in use */
	int32		  max_threads;					/* maximum # threads */
	int32		  used_threads;					/* # threads in use */
	int32		  max_teams;					/* maximum # teams */
	int32		  used_teams;					/* # teams in use */

	char		  kernel_name [SYS_MAX_NAME_LEN];		/* name of kernel */
	char          kernel_build_date[B_OS_NAME_LENGTH];	/* date kernel built */
	char          kernel_build_time[B_OS_NAME_LENGTH];	/* time kernel built */
	int64         kernel_version;             	/* version of this kernel */

	bigtime_t	  _busy_wait_time;				/* reserved for Be */
	int32         pad[4];   	               	/* just in case... */
} system_info;

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
#endif
#ifdef __cplusplus
}
#endif

#endif		/* ifdef _LIBROOT_TYPES_ */
