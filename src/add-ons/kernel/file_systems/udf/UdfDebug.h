//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  UDF version copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Initial version copyright (c) 2002 Axel Dörfler, axeld@pinc-software.de
//	dbg_printf() function copyright (c) 2003 Ingo Weinhold, bonefish@cs.tu-berlin.edu
//----------------------------------------------------------------------
#ifndef UDF_DEBUG_H
#define UDF_DEBUG_H

/*! \file UdfDebug.h

	Handy debugging macros.
*/

#include <KernelExport.h>

#include <OS.h>
#ifdef DEBUG
#	include <string.h>
#endif
#include <unistd.h>

#define DEBUG_TO_FILE 1

#if DEBUG_TO_FILE
#	include <stdio.h>
#	include <fcntl.h>
#	define __out dbg_printf		
	void dbg_printf(const char *format,...);
	void initialize_debugger(const char *filename);
#else
#	ifdef USER
#		include <stdio.h>
#		define __out printf
#	else
#		include <null.h>
#		define __out dprintf
#	endif
#endif

#include "kernel_cpp.h"

class DebugHelper;

int32 _get_debug_indent_level();

/*! \brief Categories specifiable via the categoryFlags parameter to DEBUG_INIT()
*/
enum _DebugCategoryFlags {
	// Function visibility categories
	CF_ENTRY 			= 0x00000001,	//!< fs entry functions
	CF_PUBLIC			= 0x00000002,	//!< public methods and global functions
	CF_PRIVATE			= 0x00000004,	//!< private methods
	
	// Function type categories
	CF_VOLUME_OPS		= 0x00000008,
	CF_FILE_OPS			= 0x00000010,
	CF_DIRECTORY_OPS	= 0x00000020,
	CF_ATTRIBUTE_OPS	= 0x00000040,
	CF_INDEX_OPS		= 0x00000080,
	CF_QUERY_OPS		= 0x00000100,

	
	// Misc categories
	CF_HIGH_VOLUME		= 0x00000200,	//!< often-called functions
	CF_HELPER			= 0x00000400,	//!< helper functions and classes (i.e. Array, CS0String, etc.)
	CF_DEBUGGING		= 0x00000800,	//!< internal debugging functions
	CF_DUMP				= 0x00001000,	//!< dump() functions

	// Specific entry functions
	CF_UDF_READ_FS_STAT = 0x00002000,
	CF_UDF_READ			= 0x00004000,
	
	//-------------------------------
	
	// Handy combinations
	CF_ALL				= 0xffffffff,
	CF_ALL_LOW_VOLUME	= ~CF_HIGH_VOLUME,

	CF_ANY_VISIBILITY 	= 0x00000007,
	CF_ANY_OP			= 0x000001f8,
	CF_ALL_STANDARD     = CF_ANY_VISIBILITY | CF_ANY_OP,
};

/*! \def CATEGORY_FILTER
	\brief Bitmask of currently enabled debugging categories.
*/
#define CATEGORY_FILTER	CF_ALL
//#define CATEGORY_FILTER ~CF_DIRECTORY_OPS & ~CF_HIGH_VOLUME & ~CF_HELPER
//#define CATEGORY_FILTER CF_UDF_READ
//#define CATEGORY_FILTER	~CF_DUMP
//#define CATEGORY_FILTER	~CF_DUMP & ~CF_HIGH_VOLUME
//#define CATEGORY_FILTER	CF_ALL_STANDARD
//#define CATEGORY_FILTER	CF_ENTRY
//#define CATEGORY_FILTER	(CF_ENTRY | CF_PUBLIC)
//#define CATEGORY_FILTER	(CF_ENTRY | CF_PUBLIC | CF_PRIVATE)
//#define CATEGORY_FILTER	(CF_ENTRY | CF_PUBLIC | CF_PRIVATE | CF_HIGH_VOLUME)

/*! \brief Helper class that is allocated on the stack by
	the \c DEBUG_INIT() macro. On creation, it increases the
	current indentation level by 1; on destruction, it decreases
	it by 1.
*/
class DebugHelper
{
public:
	DebugHelper(uint32 categoryFlags, const char *className = NULL, uint8 tabCount = 1);
	~DebugHelper();
	
	uint32 CategoryFlags() const { return fCategoryFlags; }
	const char* ClassName() const { return fClassName; }
	
private:
	uint32 fCategoryFlags;
	uint8 fTabCount;
	char *fClassName;
};

//----------------------------------------------------------------------
// NOTE: See UdfDebug.cpp for complete descriptions of the following
// debug macros.
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// DEBUG-independent macros
//----------------------------------------------------------------------
#define INFORM(x) { __out("udf: "); __out x; }
#ifdef USER
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
	
	#define DEBUG_INIT_SILENT(categoryFlags, className)	\
		DebugHelper _debugHelper((categoryFlags), className, 2);			

	#define DEBUG_INIT(categoryFlags, className)		\
		DEBUG_INIT_SILENT(categoryFlags, className);	\
		PRINT(("\n"));
		
	#define DEBUG_INIT_ETC(categoryFlags, className, arguments)				\
		DEBUG_INIT_SILENT(categoryFlags, className)							\
		if ((_debugHelper.CategoryFlags() & CATEGORY_FILTER)				\
		       == _debugHelper.CategoryFlags()) 							\
		{																	\
			PRINT_INDENT();													\
			if (_debugHelper.ClassName()) {									\
				__out("udf(%ld): %s::%s(", find_thread(NULL),				\
				      _debugHelper.ClassName(), __FUNCTION__);				\
			} else 	{														\
				__out("udf(%ld): %s(", find_thread(NULL), __FUNCTION__);	\
			}																\
			__out arguments;												\
			__out("):\n");													\
		}
		
	#define DUMP_INIT(categoryFlags, className)	\
		DEBUG_INIT_SILENT((categoryFlags) | CF_DUMP , className);	
				
	#define PRINT(x) { 														\
		if ((_debugHelper.CategoryFlags() & CATEGORY_FILTER)				\
		       == _debugHelper.CategoryFlags()) 							\
		{																	\
			PRINT_INDENT();													\
			if (_debugHelper.ClassName()) {									\
				__out("udf(%ld): %s::%s(): ", find_thread(NULL),			\
				      _debugHelper.ClassName(), __FUNCTION__);				\
			} else 	{														\
				__out("udf(%ld): %s(): ", find_thread(NULL), __FUNCTION__);	\
			}																\
			__out x; 														\
		} 																	\
	}

	#define LPRINT(x) { 													\
		if ((_debugHelper.CategoryFlags() & CATEGORY_FILTER)				\
		       == _debugHelper.CategoryFlags()) 							\
		{																	\
			PRINT_INDENT();													\
			if (_debugHelper.ClassName()) {									\
				__out("udf(%ld): %s::%s(): line %d: ", find_thread(NULL),	\
				      _debugHelper.ClassName(), __FUNCTION__, __LINE__);	\
			} else {														\
				__out("udf(%ld): %s(): line %d: ", find_thread(NULL),		\
				      __FUNCTION__, __LINE__);								\
			}																\
			__out x;														\
		} 																	\
	}

	#define SIMPLE_PRINT(x) { 									\
		if ((_debugHelper.CategoryFlags() & CATEGORY_FILTER)	\
		       == _debugHelper.CategoryFlags()) 				\
		{														\
			__out x; 											\
		} 														\
	}

	#define PRINT_INDENT() { 									\
		if ((_debugHelper.CategoryFlags() & CATEGORY_FILTER)	\
		       == _debugHelper.CategoryFlags()) 				\
		{														\
			int32 _level = _get_debug_indent_level();			\
			for (int32 i = 0; i < _level-1; i++) {				\
				__out(" ");										\
			}													\
		}														\
	}
	
	#define PRINT_DIVIDER()	\
		PRINT_INDENT(); 	\
		SIMPLE_PRINT(("------------------------------------------------------------\n"));
		
	#define DUMP(object)												\
		if ((_debugHelper.CategoryFlags() & CATEGORY_FILTER)	\
		       == _debugHelper.CategoryFlags()) 				\
		{														\
			(object).dump();											\
		}		
	
	#define PDUMP(objectPointer)								\
		if ((_debugHelper.CategoryFlags() & CATEGORY_FILTER)	\
		       == _debugHelper.CategoryFlags()) 				\
		{														\
			(objectPointer)->dump();								\
		}		
	
	#define REPORT_ERROR(err) {											\
		LPRINT(("returning error 0x%lx, `%s'\n", err, strerror(err)));	\
	}

	#define RETURN_ERROR(err) { 		\
		status_t _status = err; 		\
		if (_status < (status_t)B_OK)	\
			REPORT_ERROR(_status);		\
		return _status;					\
	}

	#define RETURN(err) { 											\
		status_t _status = err; 									\
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
	#define DEBUG_INIT_SILENT(categoryFlags, className)	;
	#define DEBUG_INIT(categoryFlags, className) ;
	#define DEBUG_INIT_ETC(categoryFlags, className, arguments) ;
	#define DUMP_INIT(categoryFlags, className)	;
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

#endif	// DEBUG_H 
