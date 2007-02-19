#ifndef DEBUG_H
#define DEBUG_H
/* Debug - debug stuff
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/
// Modified by Ingo Weinhold (bonefish@cs.tu-berlin.de)

#include <string.h>

#include <OS.h>
#include <SupportDefs.h>

// define all macros we work with -- undefined macros are set default
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
#ifndef DEBUG_PRINT_FILE
#	define DEBUG_PRINT_FILE "/var/log/reiserfs.log"
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

// functions exported by this module
#if DEBUG_PRINT
	status_t init_debugging();
	status_t exit_debugging();
	void dbg_printf(const char *format,...);
#else
	static inline status_t init_debugging() { return B_OK; }
	static inline status_t exit_debugging() { return B_OK; }
	static inline void dbg_printf(const char */*format*/,...) {}
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

#define DEBUG_APP "ramfs"
#if DEBUG
	#define PRINT(x) { __out(DEBUG_APP ": "); __out x; }
	#define REPORT_ERROR(status) __out(DEBUG_APP ": %s:%d: %s\n",__FUNCTION__,__LINE__,strerror(status));
	#define RETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); return _status;}
	#define SET_ERROR(var, err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); var = _status; }
	#define FATAL(x) { __out(DEBUG_APP ": FATAL: "); __out x; }
	#define INFORM(x) { __out(DEBUG_APP ": "); __out x; }
	#define FUNCTION(x) { __out(DEBUG_APP ": %s() ",__FUNCTION__); __out x; }
	#define FUNCTION_START() { __out(DEBUG_APP ": %s()\n",__FUNCTION__); }
	#define FUNCTION_END() { __out(DEBUG_APP ": %s() done\n",__FUNCTION__); }
	#define D(x) {x;};
	#define DARG(x) x
#else
	#define PRINT(x) ;
	#define REPORT_ERROR(status) ;
	#define RETURN_ERROR(status) return status;
	#define SET_ERROR(var, err) var = err;
	#define FATAL(x) { __out(DEBUG_APP ": FATAL: "); __out x; }
	#define INFORM(x) { __out(DEBUG_APP ": "); __out x; }
	#define FUNCTION(x) ;
	#define FUNCTION_START() ;
	#define FUNCTION_END() ;
	#define D(x) ;
	#define DARG(x)
#endif

#define TPRINT(x) { __out(DEBUG_APP ": "); __out x; }
#define TREPORT_ERROR(status) __out(DEBUG_APP ": %s:%d: %s\n",__FUNCTION__,__LINE__,strerror(status));
#define TRETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); return _status;}
#define TFUNCTION(x) { __out(DEBUG_APP ": %s() ",__FUNCTION__); __out x; }
#define TFUNCTION_START() { __out(DEBUG_APP ": %s()\n",__FUNCTION__); }
#define TFUNCTION_END() { __out(DEBUG_APP ": %s() done\n",__FUNCTION__); }

#if USER
	#define DEBUGGER(x)	debugger(x);
#else
//	#define DEBUGGER(x)
	#define DEBUGGER(x)	debugger(x);
#endif

#endif	/* DEBUG_H */
