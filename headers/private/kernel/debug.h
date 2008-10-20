/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the Haiku License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#include "kernel_debug_config.h"

#include <KernelExport.h>
#include <module.h>


/*	KDEBUG
	The kernel debug level.
	Level 1 is usual asserts, > 1 should be used for very expensive runtime
	checks
 */
#if !defined(KDEBUG)
#	if DEBUG
#		define KDEBUG 1
#	else
#		define KDEBUG 0
#	endif
#endif

#define ASSERT_ALWAYS(x) \
	do { if (!(x)) { panic("ASSERT FAILED (%s:%d): %s\n", __FILE__, __LINE__, #x); } } while (0)

#define ASSERT_ALWAYS_PRINT(x, format...) \
	do {																	\
		if (!(x)) {															\
			dprintf_no_syslog(format);										\
			panic("ASSERT FAILED (%s:%d): %s\n", __FILE__, __LINE__, #x);	\
		}																	\
	} while (0)

#if KDEBUG
#	define ASSERT(x)					ASSERT_ALWAYS(x)
#	define ASSERT_PRINT(x, format...)	ASSERT_ALWAYS_PRINT(x, format)
#else
#	define ASSERT(x)					do { } while(0)
#	define ASSERT_PRINT(x, format...)	do { } while(0)
#endif

#if KDEBUG
#	define KDEBUG_ONLY(x)				x
#else
#	define KDEBUG_ONLY(x)				/* nothing */
#endif

// command return value
#define B_KDEBUG_ERROR			4
#define B_KDEBUG_RESTART_PIPE	5

// command flags
#define B_KDEBUG_DONT_PARSE_ARGUMENTS	(0x01)
#define B_KDEBUG_PIPE_FINAL_RERUN		(0x02)

struct debugger_module_info {
	module_info info;

	void (*enter_debugger)(void);
	void (*exit_debugger)(void);

	// io hooks
	int (*debugger_puts)(const char *str, int32 length);
	int (*debugger_getchar)(void);
	// TODO: add hooks for tunnelling gdb ?
};

struct debugger_demangle_module_info {
	module_info info;

	const char* (*demangle_symbol)(const char* name, char* buffer,
		size_t bufferSize, bool* _isObjectMethod);
	status_t (*get_next_argument)(uint32* _cookie, const char* symbol,
		char* name, size_t nameSize, int32* _type, size_t* _argumentLength);
};


extern int dbg_register_file[B_MAX_CPU_COUNT][14];

typedef struct debug_page_fault_info {
	addr_t	fault_address;
	addr_t	pc;
	uint32	flags;
} debug_page_fault_info;

// debug_page_fault_info::flags
#define DEBUG_PAGE_FAULT_WRITE		0x01	/* write fault */
#define DEBUG_PAGE_FAULT_NO_INFO	0x02	/* fault address and read/write
											   unknown */


#ifdef __cplusplus
extern "C" {
#endif

struct kernel_args;

extern status_t debug_init(struct kernel_args *args);
extern status_t	debug_init_post_vm(struct kernel_args *args);
extern status_t	debug_init_post_modules(struct kernel_args *args);
extern void		debug_early_boot_message(const char *string);
extern void		debug_puts(const char *s, int32 length);
extern bool		debug_debugger_running(void);
extern bool		debug_screen_output_enabled(void);
extern void		debug_stop_screen_debug_output(void);
extern void		debug_set_page_fault_info(addr_t faultAddress, addr_t pc,
					uint32 flags);
extern debug_page_fault_info* debug_get_page_fault_info();
extern void debug_trap_cpu_in_kdl(bool returnIfHandedOver);

extern void		kputs(const char *string);
extern void		kputs_unfiltered(const char *string);
extern void		kprintf_unfiltered(const char *format, ...)
					__attribute__ ((format (__printf__, 1, 2)));
extern void		dprintf_no_syslog(const char *format, ...)
					__attribute__ ((format (__printf__, 1, 2)));

extern bool		is_debug_variable_defined(const char* variableName);
extern bool		set_debug_variable(const char* variableName, uint64 value);
extern uint64	get_debug_variable(const char* variableName,
					uint64 defaultValue);
extern bool		unset_debug_variable(const char* variableName);
extern void		unset_all_debug_variables();

extern bool		evaluate_debug_expression(const char* expression,
					uint64* result, bool silent);
extern int		evaluate_debug_command(const char* command);
extern status_t	parse_next_debug_command_argument(const char** expressionString,
					char* buffer, size_t bufferSize);

extern status_t	add_debugger_command_etc(const char* name,
					debugger_command_hook func, const char* description,
					const char* usage, uint32 flags);
extern status_t	add_debugger_command_alias(const char* newName,
					const char* oldName, const char* description);
extern bool		print_debugger_command_usage(const char* command);

extern const char *debug_demangle_symbol(const char* symbol, char* buffer,
					size_t bufferSize, bool* _isObjectMethod);
extern status_t	debug_get_next_demangled_argument(uint32* _cookie,
					const char* symbol, char* name, size_t nameSize,
					int32* _type, size_t* _argumentLength);

extern struct thread* debug_set_debugged_thread(struct thread* thread);
extern struct thread* debug_get_debugged_thread();

extern void		_user_debug_output(const char *userString);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_DEBUG_H */
