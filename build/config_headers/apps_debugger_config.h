#ifndef APPS_DEBUGGER_CONFIG_H
#define APPS_DEBUGGER_CONFIG_H


// trace DWARF debug info entry parsing
#define	APPS_DEBUGGER_TRACE_DWARF_DIE			0

// trace DWARF line info:
// 1: general info only
// 2: line number program execution
#define	APPS_DEBUGGER_TRACE_DWARF_LINE_INFO		0

// trace DWARF expression evaluation
#define	APPS_DEBUGGER_TRACE_DWARF_EXPRESSIONS	0

// dump DWARF public types section
#define	APPS_DEBUGGER_TRACE_DWARF_PUBLIC_TYPES	0

// trace (DWARF) canonical frame info parsing/evaluation
#define	APPS_DEBUGGER_TRACE_CFI					0

// trace retrieving of stack frame local variable types and values
#define	APPS_DEBUGGER_TRACE_STACK_FRAME_LOCALS	0

// trace image loading and changes
#define	APPS_DEBUGGER_TRACE_IMAGES				0

// trace program code reading/analyzing
#define	APPS_DEBUGGER_TRACE_CODE				0

// trace general job handling
#define	APPS_DEBUGGER_TRACE_JOBS				0

// trace debug events
#define	APPS_DEBUGGER_TRACE_DEBUG_EVENTS		0

// trace controlling the debugged team (stepping, breakpoints,...)
#define	APPS_DEBUGGER_TRACE_TEAM_CONTROL		0

// trace GUI operations
#define	APPS_DEBUGGER_TRACE_GUI					0


#endif	// APPS_DEBUGGER_CONFIG_H
