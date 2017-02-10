//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  This version copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Initial version copyright (c) 2002 Axel Dörfler, axeld@pinc-software.de
//	dbg_printf() function copyright (c) 2003 Ingo Weinhold, bonefish@cs.tu-berlin.edu
//----------------------------------------------------------------------
#ifndef _UDF_DEBUG_H
#define _UDF_DEBUG_H

/*! \file Debug.h

	Handy debugging macros.
*/

#include <KernelExport.h>

#include <OS.h>
#ifdef DEBUG
//#	include <string.h>
#endif
#include <unistd.h>


#define DEBUG_TO_FILE 0

#	include <stdio.h>
#if DEBUG_TO_FILE
//#	include <stdio.h>
#	include <stdarg.h>
extern "C" int 	vsprintf(char *s, const char *format, va_list arg);
#	include <fcntl.h>
#	define __out dbg_printf		
	void dbg_printf(const char *format,...);
	void initialize_debugger(const char *filename);
#else
#	if !_KERNEL_MODE 
//#		include <stdio.h> 
#		define __out printf 
#	else 
//#		include <null.h> 
#		define __out dprintf
#	endif
#	include <stdio.h>
#	include <malloc.h>
//#	define __out printf
#endif

#include "kernel_cpp.h"

class DebugHelper;

int32 _get_debug_indent_level();

/*! \brief Helper class that is allocated on the stack by
	the \c DEBUG_INIT() macro. On creation, it increases the
	current indentation level by the amount specified via its
	constructor's \c tabCount parameter; on destruction, it
	decreases it.
*/
class DebugHelper
{
public:
	DebugHelper(const char *className = NULL, uint8 tabCount = 1);
	~DebugHelper();
	
	uint8 TabCount() const { return fTabCount; }
	const char* ClassName() const { return fClassName; }
	
private:
	uint8 fTabCount;
	char *fClassName;
};

//----------------------------------------------------------------------
// NOTE: See Debug.cpp for complete descriptions of the following
// debug macros.
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// DEBUG-independent macros
//----------------------------------------------------------------------
#define INFORM(x) { __out("udf: "); __out x; }
#if !_KERNEL_MODE
#	define DIE(x) debugger x
#else
#	define DIE(x) kernel_debugger x
#endif

//----------------------------------------------------------------------
// DEBUG-dependent macros
//----------------------------------------------------------------------
#ifdef DEBUG
	#if DEBUG_TO_FILE
		#define INITIALIZE_DEBUGGING_OUTPUT_FILE(filename) initialize_debugger(filename);
	#else
		#define INITIALIZE_DEBUGGING_OUTPUT_FILE(filename) ;
	#endif
	
	#define DEBUG_INIT_SILENT(className)			\
		DebugHelper _debugHelper(className, 2);			

	#define DEBUG_INIT(className)		\
		DEBUG_INIT_SILENT(className);	\
		PRINT(("\n"));
		
	#define DEBUG_INIT_ETC(className, arguments)							\
		DEBUG_INIT_SILENT(className)										\
		{																	\
			PRINT_INDENT();													\
			if (_debugHelper.ClassName()) {									\
				__out("udf: %s::%s(", 									\
				      _debugHelper.ClassName(), __FUNCTION__);				\
			} else 	{														\
				__out("udf: %s(", __FUNCTION__);						\
			}																\
			__out arguments;												\
			__out("):\n");													\
		}
		
	#define DUMP_INIT(className)	\
		DEBUG_INIT_SILENT(className);	
				
	#define PRINT(x) { 														\
		{																	\
			PRINT_INDENT();													\
			if (_debugHelper.ClassName()) {									\
				__out("udf: %s::%s(): ", 								\
				      _debugHelper.ClassName(), __FUNCTION__);				\
			} else 	{														\
				__out("udf: %s(): ",  __FUNCTION__);					\
			}																\
			__out x; 														\
		} 																	\
	}

	#define LPRINT(x) { 													\
		{																	\
			PRINT_INDENT();													\
			if (_debugHelper.ClassName()) {									\
				__out("udf: %s::%s(): line %d: ", 						\
				      _debugHelper.ClassName(), __FUNCTION__, __LINE__);	\
			} else {														\
				__out("udf: %s(): line %d: ", 							\
				      __FUNCTION__, __LINE__);								\
			}																\
			__out x;														\
		} 																	\
	}

	#define SIMPLE_PRINT(x) { 									\
		{														\
			__out x; 											\
		} 														\
	}

	#define PRINT_INDENT() { 									\
		{														\
			int32 _level = _get_debug_indent_level();			\
			for (int32 i = 0; i < _level-_debugHelper.TabCount(); i++) {				\
				__out(" ");										\
			}													\
		}														\
	}
	
	#define PRINT_DIVIDER()	\
		PRINT_INDENT(); 	\
		SIMPLE_PRINT(("------------------------------------------------------------\n"));
		
	#define DUMP(object)				\
		{								\
			(object).dump();			\
		}		
	
	#define PDUMP(objectPointer)		\
		{								\
			(objectPointer)->dump();	\
		}		
	
	#define REPORT_ERROR(error) {											\
		LPRINT(("returning error 0x%lx, `%s'\n", error, strerror(error)));	\
	}

	#define RETURN_ERROR(error) { 		\
		status_t _status = error; 		\
		if (_status < (status_t)B_OK)	\
			REPORT_ERROR(_status);		\
		return _status;					\
	}

	#define RETURN(error) { 										\
		status_t _status = error; 									\
		if (_status < (status_t)B_OK) {								\
			REPORT_ERROR(_status); 									\
		} else if (_status == (status_t)B_OK) {						\
			LPRINT(("returning B_OK\n"));							\
		} else {													\
			LPRINT(("returning 0x%lx = %ld\n", _status, _status));	\
		}															\
		return _status; 											\
	}

	#define FATAL(x) { 								\
		PRINT(("fatal error: ")); SIMPLE_PRINT(x);	\
	}
	
	#define DBG(x) x ;
	
#else	// ifdef DEBUG
	#define INITIALIZE_DEBUGGING_OUTPUT_FILE(filename) ;
	#define DEBUG_INIT_SILENT(className)	;
	#define DEBUG_INIT(className) ;
	#define DEBUG_INIT_ETC(className, arguments) ;
	#define DUMP_INIT(className)	;
	#define PRINT(x) ;
	#define LPRINT(x) ;
	#define SIMPLE_PRINT(x) ;
	#define PRINT_INDENT(x) ;
	#define PRINT_DIVIDER()	;
	#define DUMP(object) ;
	#define PDUMP(objectPointer) ;
	#define REPORT_ERROR(status) ;
	#define RETURN_ERROR(status) return status;
	#define RETURN(status) return status;
	#define FATAL(x) { __out("udf: fatal error: "); __out x; }
	#define DBG(x) ;
	#define DUMP(x) ;
#endif	// ifdef DEBUG else

#define TRACE(x) DBG(dprintf x)

// These macros turn on or off extensive and generally unnecessary
// debugging output regarding table of contents parsing
//#define WARN(x) (dprintf x)
//#define WARN(x)
#define WARN(x) DBG(dprintf x)

#endif	// _UDF_DEBUG_H
