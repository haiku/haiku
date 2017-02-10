#ifndef DEBUG_H
#define DEBUG_H
/* Debug - debug stuff
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/
#include <string.h>

#if !USER
#	include <KernelExport.h>
#endif
#include <OS.h>
#include <SupportDefs.h>

// define all macros we work with -- undefined macros are set to defaults
#ifndef USER
#	define USER 0
#endif
#ifndef DEBUG
#	define DEBUG 0
#endif
#if !DEBUG
#	undef DEBUG_PRINT
#	define DEBUG_PRINT 0
#endif
#ifndef DEBUG_PRINT
#	define DEBUG_PRINT 0
#endif
#ifndef DEBUG_APP
#	define DEBUG_APP	"debug"
#endif
#ifndef DEBUG_PRINT_FILE
#	define DEBUG_PRINT_FILE "/var/log/" DEBUG_APP ".log"
#endif

// define the debug output function
#if USER
#	include <stdio.h>
#	if DEBUG_PRINT
#		define __out dbg_printf
#	else
#		define __out printf
#	endif
#else
#	include <KernelExport.h>
#	include <null.h>
#	if DEBUG_PRINT
#		define __out dbg_printf
#	else
#		define __out dprintf
#	endif
#endif

// define the PANIC() macro
#ifndef PANIC
#	if USER
#		define PANIC(str)	debugger(str)
#	else
#		define PANIC(str)	panic(str)
#	endif
#endif

// functions exported by this module
status_t init_debugging();
status_t exit_debugging();
void dbg_printf_begin();
void dbg_printf_end();
#if DEBUG_PRINT
	void dbg_printf(const char *format,...);
#else
	static inline void dbg_printf(const char *,...) {}
#endif

// Short overview over the debug output macros:
//	PRINT()
//		is for general messages that very unlikely should appear in a release build
//	FATAL()
//		this is for fatal messages, when something has really gone wrong
//	INFORM()
//		general information, as disk size, etc.
//	REPORT_ERROR(status_t)
//		prints out error information
//	RETURN_ERROR(status_t)
//		calls REPORT_ERROR() and return the value
//	D()
//		the statements in D() are only included if DEBUG is defined

#if __MWERKS__
#	define __FUNCTION__	""
#endif

#define DEBUG_THREAD	find_thread(NULL)
#define DEBUG_CONTEXT(x)	{ dbg_printf_begin(); __out(DEBUG_APP " [%Ld: %5ld] ", system_time(), DEBUG_THREAD); x; dbg_printf_end(); }
#define DEBUG_CONTEXT_FUNCTION(prefix, x)	{ dbg_printf_begin(); __out(DEBUG_APP " [%Ld: %5ld] %s()" prefix, system_time(), DEBUG_THREAD, __FUNCTION__); x; dbg_printf_end(); }
#define DEBUG_CONTEXT_LINE(x)	{ dbg_printf_begin(); __out(DEBUG_APP " [%Ld: %5ld] %s():%d: ", system_time(), DEBUG_THREAD, __FUNCTION__, __LINE__); x; dbg_printf_end(); }

#define TPRINT(x) DEBUG_CONTEXT( __out x )
#define TREPORT_ERROR(status) DEBUG_CONTEXT_LINE( __out("%s\n", strerror(status)) )
#define TRETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) TREPORT_ERROR(_status); return _status;}
#define TSET_ERROR(var, err) { status_t _status = err; if (_status < B_OK) TREPORT_ERROR(_status); var = _status; }
#define TFUNCTION(x) DEBUG_CONTEXT_FUNCTION( ": ", __out x )
#define TFUNCTION_START() DEBUG_CONTEXT_FUNCTION( "\n",  )
#define TFUNCTION_END() DEBUG_CONTEXT_FUNCTION( " done\n",  )

#if DEBUG
	#define PRINT(x) TPRINT(x)
	#define REPORT_ERROR(status) TREPORT_ERROR(status)
	#define RETURN_ERROR(err) TRETURN_ERROR(err)
	#define SET_ERROR(var, err) TSET_ERROR(var, err)
	#define FATAL(x) DEBUG_CONTEXT( __out x )
	#define ERROR(x) DEBUG_CONTEXT( __out x )
	#define WARN(x) DEBUG_CONTEXT( __out x )
	#define INFORM(x) DEBUG_CONTEXT( __out x )
	#define FUNCTION(x) TFUNCTION(x)
	#define FUNCTION_START() TFUNCTION_START()
	#define FUNCTION_END() TFUNCTION_END()
	#define DARG(x) x
	#define D(x) {x;};
#else
	#define PRINT(x) ;
	#define REPORT_ERROR(status) ;
	#define RETURN_ERROR(status) return status;
	#define SET_ERROR(var, err) var = err;
	#define FATAL(x) DEBUG_CONTEXT( __out x )
	#define ERROR(x) DEBUG_CONTEXT( __out x )
	#define WARN(x) DEBUG_CONTEXT( __out x )
	#define INFORM(x) DEBUG_CONTEXT( __out x )
	#define FUNCTION(x) ;
	#define FUNCTION_START() ;
	#define FUNCTION_END() ;
	#define DARG(x)
	#define D(x) ;
#endif

#ifndef TOUCH
#define TOUCH(var) (void)var
#endif

#endif	/* DEBUG_H */
