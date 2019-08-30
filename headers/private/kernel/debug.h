/*
 * Copyright 2002-2015, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H


#include <setjmp.h>

#include <KernelExport.h>
#include <module.h>

#include "kernel_debug_config.h"


// We need the BKernel::Thread type below (opaquely) in the exported C
// functions below. Since this header is currently still included by plain C
// code, we define a dummy type BKernel_Thread in C mode and a equally named
// macro in C++ mode.
#ifdef __cplusplus
	namespace BKernel {
		struct Thread;
	}

	using BKernel::Thread;
#	define BKernel_Thread	Thread
#else
	typedef struct BKernel_Thread BKernel_Thread;
#endif


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
	do {																	\
		if (!(x)) {															\
			panic("ASSERT FAILED (%s:%d): %s", __FILE__, __LINE__, #x);		\
		}																	\
	} while (0)

#define ASSERT_ALWAYS_PRINT(x, format, args...) \
	do {																	\
		if (!(x)) {															\
			panic("ASSERT FAILED (%s:%d): %s; " format, __FILE__, __LINE__,	\
				#x, args);													\
		}																	\
	} while (0)

#if KDEBUG
#	define ASSERT(x)						ASSERT_ALWAYS(x)
#	define ASSERT_PRINT(x, format, args...)	ASSERT_ALWAYS_PRINT(x, format, args)
#else
#	define ASSERT(x)						do { } while(0)
#	define ASSERT_PRINT(x, format, args...)	do { } while(0)
#endif

#if __GNUC__ >= 5 && !defined(__cplusplus)
#	define STATIC_ASSERT(x) _Static_assert(x, "static assert failed!")
#elif defined(__cplusplus) && __cplusplus >= 201103L
#	define STATIC_ASSERT(x) static_assert(x, "static assert failed!")
#else
#	define STATIC_ASSERT(x)								\
		do {												\
			struct __staticAssertStruct__ {					\
				char __static_assert_failed__[2*(x) - 1];	\
			};												\
		} while (false)
#endif

#if KDEBUG
#	define KDEBUG_ONLY(x)				x
#else
#	define KDEBUG_ONLY(x)				/* nothing */
#endif


// Macros for for placing marker functions. They can be used to mark the
// beginning and end of code sections (e.g. used in the slab code).
#define RANGE_MARKER_FUNCTION(functionName) \
	void functionName() {}
#define RANGE_MARKER_FUNCTION_BEGIN(scope) \
	RANGE_MARKER_FUNCTION(scope##_begin)
#define RANGE_MARKER_FUNCTION_END(scope) \
	RANGE_MARKER_FUNCTION(scope##_end)

#define RANGE_MARKER_FUNCTION_PROTOTYPE(functionName) \
	void functionName();
#define RANGE_MARKER_FUNCTION_PROTOTYPES(scope)		\
	RANGE_MARKER_FUNCTION_PROTOTYPE(scope##_begin)	\
	RANGE_MARKER_FUNCTION_PROTOTYPE(scope##_end)
#define RANGE_MARKER_FUNCTION_ADDRESS_RANGE(scope) \
	(addr_t)&scope##_begin, (addr_t)&scope##_end


// command return value
#define B_KDEBUG_ERROR			4
#define B_KDEBUG_RESTART_PIPE	5

// command flags
#define B_KDEBUG_DONT_PARSE_ARGUMENTS	(0x01)
#define B_KDEBUG_PIPE_FINAL_RERUN		(0x02)


struct arch_debug_registers;


struct debugger_module_info {
	module_info info;

	void (*enter_debugger)(void);
	void (*exit_debugger)(void);

	// I/O hooks
	int (*debugger_puts)(const char *str, int32 length);
	int (*debugger_getchar)(void);
	// TODO: add hooks for tunnelling gdb ?

	// Misc. hooks
	bool (*emergency_key_pressed)(char key);
};

struct debugger_demangle_module_info {
	module_info info;

	const char* (*demangle_symbol)(const char* name, char* buffer,
		size_t bufferSize, bool* _isObjectMethod);
	status_t (*get_next_argument)(uint32* _cookie, const char* symbol,
		char* name, size_t nameSize, int32* _type, size_t* _argumentLength);
};


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

extern void		debug_init(struct kernel_args *args);
extern void		debug_init_post_vm(struct kernel_args *args);
extern void		debug_init_post_settings(struct kernel_args *args);
extern void		debug_init_post_modules(struct kernel_args *args);
extern void		debug_early_boot_message(const char *string);
extern void		debug_puts(const char *s, int32 length);
extern bool		debug_debugger_running(void);
extern bool		debug_screen_output_enabled(void);
extern void		debug_stop_screen_debug_output(void);
extern void		debug_set_page_fault_info(addr_t faultAddress, addr_t pc,
					uint32 flags);
extern debug_page_fault_info* debug_get_page_fault_info();
extern void		debug_trap_cpu_in_kdl(int32 cpu, bool returnIfHandedOver);
extern void		debug_double_fault(int32 cpu);
extern bool		debug_emergency_key_pressed(char key);
extern bool		debug_is_kernel_memory_accessible(addr_t address, size_t size,
					uint32 protection);
extern int		debug_call_with_fault_handler(jmp_buf jumpBuffer,
					void (*function)(void*), void* parameter);
extern status_t	debug_memcpy(team_id teamID, void* to, const void* from,
					size_t size);
extern ssize_t	debug_strlcpy(team_id teamID, char* to, const char* from,
					size_t size);

extern char		kgetc(void);
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
extern bool		has_debugger_command(const char* command);

extern const char *debug_demangle_symbol(const char* symbol, char* buffer,
					size_t bufferSize, bool* _isObjectMethod);
extern status_t	debug_get_next_demangled_argument(uint32* _cookie,
					const char* symbol, char* name, size_t nameSize,
					int32* _type, size_t* _argumentLength);

extern BKernel_Thread* debug_set_debugged_thread(BKernel_Thread* thread);
extern BKernel_Thread* debug_get_debugged_thread();
extern bool debug_is_debugged_team(team_id teamID);

extern struct arch_debug_registers* debug_get_debug_registers(int32 cpu);

extern status_t _user_kernel_debugger(const char *message);
extern void _user_register_syslog_daemon(port_id port);
extern void	_user_debug_output(const char *userString);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

struct DebuggedThreadSetter {
	DebuggedThreadSetter(Thread* thread)
		:
		fPreviousThread(debug_set_debugged_thread(thread))
	{
	}

	~DebuggedThreadSetter()
	{
		debug_set_debugged_thread(fPreviousThread);
	}

private:
	Thread*	fPreviousThread;
};


#endif	// __cplusplus


#endif	/* _KERNEL_DEBUG_H */
