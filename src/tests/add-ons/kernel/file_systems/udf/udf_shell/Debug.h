#ifndef DEBUG_H
#define DEBUG_H
/* Debug - debug stuff
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** UDF version by Tyler Dauwalder, tyler@dauwalder.net
** This file may be used under the terms of the MIT License.
*/

#ifdef DEBUG
#	include <string.h>
#endif

#ifdef USER
#	include <stdio.h>
#	define __out printf
#else
#	define __out dprintf
#endif

// Which debugger should be used when?
// The DEBUGGER() macro actually has no effect if DEBUG is not defined,
// use the DIE() macro if you really want to die.
#ifdef DEBUG
#	ifdef USER
#		define DEBUGGER(x) debugger x
#	else
#		define DEBUGGER(x) kernel_debugger x
#	endif
#else
#	define DEBUGGER(x) ;
#endif

#ifdef USER
#	define DIE(x) debugger x
#else
#	define DIE(x) kernel_debugger x
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

	#include <KernelExport.h>
	#define kprintf printf
	#define dprintf printf

#ifdef DEBUG
	#define PRINT(x) { __out("udf: "); __out x; }
	#define REPORT_ERROR(status) __out("udf: %s:%s:%ld: %s\n", __FILE__, __FUNCTION__, __LINE__, strerror(status));
	#define RETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); return _status;}
	#define FATAL(x) { __out("udf: "); __out x; }
	#define INFORM(x) { __out("udf: "); __out x; }
	#define FUNCTION() __out("udf: %s()\n",__FUNCTION__);
	#define FUNCTION_START(x) { __out("udf: %s() ",__FUNCTION__); __out x; }
//	#define FUNCTION() ;
//	#define FUNCTION_START(x) ;
	#define D(x) {x;};
	#define ASSERT(x) { if (!(x)) DEBUGGER(("udf: assert failed: " #x "\n")); }
#else
	#define PRINT(x) ;
	#define REPORT_ERROR(status) ;
	#define RETURN_ERROR(status) return status;
	#define FATAL(x) { __out("udf: "); __out x; }
	#define INFORM(x) { __out("udf: "); __out x; }
	#define FUNCTION() ;
	#define FUNCTION_START(x) ;
	#define D(x) ;
	#define ASSERT(x) ;
#endif

#ifdef DEBUG
#endif

#endif	/* DEBUG_H */
