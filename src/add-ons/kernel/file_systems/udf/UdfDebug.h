//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  UDF version copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Initial version copyright (c) 2002 Axel Dörfler, axeld@pinc-software.de
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

#ifdef USER
#	include <stdio.h>
#	define __out printf
#else
#	include <null.h>
#	define __out dprintf
#endif

/*! \def DIE(x)
	\brief Drops the user into the appropriate debugger (user or kernel)
	after printing out the handy message bundled in the parenthesee
	enclosed printf-style format string found in \a x.

	\param x A printf-style format string enclosed in an extra set of parenteses,
	         e.g. PRINT(("%d\n", 0));	
*/
#ifdef USER
#	define DIE(x) debugger x
#else
#	define DIE(x) kernel_debugger x
#endif

int32 _get_debug_indent_level();

/*! \brief Helper class that is allocated on the stack by
	the \c DEBUG_INIT() macro. On creation, it increases the
	current indentation level by 1; on destruction, it decreases
	it by 1.
*/
class _DebugHelper
{
public:
	_DebugHelper();
	~_DebugHelper();
};

//----------------------------------------------------------------------
// Long-winded overview of the debug output macros:
//----------------------------------------------------------------------
/*! \def DEBUG_INIT()
	\brief Increases the indentation level, prints out the enclosing function's
	name, and creates a \c _DebugHelper object on the stack to automatically
	decrease the indentation level upon function exit.
	
	This macro should be called at the very beginning of any function in
	which you wish to use any of the other debugging macros.
	
	If DEBUG is undefined, does nothing.
*/
//----------------------------------------------------------------------
/*! \def PRINT(x)
	\brief Prints out the enclosing function's name followed by the contents
	of \a x at the current indentation level.
	
	\param x A printf-style format string enclosed in an extra set of parenteses,
	         e.g. PRINT(("%d\n", 0));
	
	If DEBUG is undefined, does nothing.
*/
//----------------------------------------------------------------------
/*! \def LPRINT(x)
	\brief Identical to \c PRINT(x), except that the line number in the source
	file at which the macro is invoked is also printed.
	
	\param x A printf-style format string enclosed in an extra set of parenteses,
	         e.g. PRINT(("%d\n", 0));
	
	If DEBUG is undefined, does nothing.
*/
//----------------------------------------------------------------------
/*! \def SIMPLE_PRINT(x)
	\brief Directly prints the contents of \a x with no extra formatting or
	information included (just like a straight \c printf() call).

	\param x A printf-style format string enclosed in an extra set of parenteses,
	         e.g. PRINT(("%d\n", 0));

	If DEBUG is undefined, does nothing.
*/	         
//----------------------------------------------------------------------
/*! \def PRINT_INDENT()
	\brief Prints out enough indentation characters to indent the current line
	to the current indentation level (assuming the cursor was flush left to
	begin with...).
	
	This function is called by the other \c *PRINT* macros, and isn't really
	intended for general consumption, but you might find it useful.
	
	If DEBUG is undefined, does nothing.
*/
//----------------------------------------------------------------------
/*! \def REPORT_ERROR(err)
	\brief Calls \c LPRINT(x) with a format string listing the error
	code in \c err (assumed to be a \c status_t value) and the
	corresponding text error code returned by a call to \c strerror().
	
	This function is called by the \c RETURN* macros, and isn't really
	intended for general consumption, but you might find it useful.
	
	\param err A \c status_t error code to report.
	
	If DEBUG is undefined, does nothing.
*/
//----------------------------------------------------------------------
/*! \def RETURN_ERROR(err)
	\brief Calls \c REPORT_ERROR(err) if err is a an error code (i.e.
	negative), otherwise remains silent. In either case, the enclosing
	function is then exited with a call to \c "return err;".
		
	\param err A \c status_t error code to report (if negative) and return.
	
	If DEBUG is undefined, silently returns the value in \c err.
*/
//----------------------------------------------------------------------
/*! \def RETURN(err)
	\brief Prints out a description of the error code being returned
	(which, in this case, may be either "erroneous" or "successful")
	and then exits the enclosing function with a call to \c "return err;".
		
	\param err A \c status_t error code to report and return.
	
	If DEBUG is undefined, silently returns the value in \c err.
*/
//----------------------------------------------------------------------
/*! \def FATAL(x)
	\brief Prints out a fatal error message.
	
	This one's still a work in progress...

	\param x A printf-style format string enclosed in an extra set of parenteses,
	         e.g. PRINT(("%d\n", 0));
	
	If DEBUG is undefined, does nothing.
*/
//----------------------------------------------------------------------
/*! \def INFORM(x)
	\brief Directly prints the contents of \a x with no extra formatting or
	information included (just like a straight \c printf() call). Does so
	whether \c DEBUG is defined or not.

	\param x A printf-style format string enclosed in an extra set of parenteses,
	         e.g. PRINT(("%d\n", 0));

	I'll say it again: Prints its output regardless to DEBUG being defined or
	undefined.
*/
//----------------------------------------------------------------------
/*! \def DBG(x)
	\brief If debug is defined, \a x is passed along to the code and
	executed unmodified. If \c DEBUG is undefined, the contents of
	\a x disappear into the ether.
	
	\param x Damn near anything resembling valid C\C++.
*/

//----------------------------------------------------------------------
// DEBUG-independent macros
//----------------------------------------------------------------------
#define INFORM(x) { __out("udf: "); __out x; }

//----------------------------------------------------------------------
// DEBUG-dependent macros
//----------------------------------------------------------------------
#ifdef DEBUG
	#define DEBUG_INIT()					\
		_DebugHelper _debug_helper_object;	\
		PRINT(("\n"));

	#define PRINT(x) {												\
		PRINT_INDENT();												\
		__out("udf(%ld): %s(): ", find_thread(NULL), __FUNCTION__);	\
		__out x; 													\
	}

	#define LPRINT(x) {																	\
		PRINT_INDENT();																	\
		__out("udf(%ld): %s(): line %d: ", find_thread(NULL), __FUNCTION__, __LINE__);	\
		__out x;																		\
	}

	#define SIMPLE_PRINT(x) { __out x; }

	#define PRINT_INDENT() {								\
		int32 _level = _get_debug_indent_level();			\
		for (int32 i = 0; i < _level-1; i++)				\
			__out(" ");										\
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
	#define DEBUG_INIT() ;
	#define PRINT(x) ;
	#define LPRINT(x) ;
	#define SIMPLE_PRINT(x) ;
	#define PRINT_INDENT(x) ;
	#define REPORT_ERROR(status) ;
	#define RETURN_ERROR(status) return status;
	#define RETURN(status) return status;
	#define FATAL(x) { __out("udf: fatal error: "); __out x; }
	#define DBG(x) ;
#endif	// ifdef DEBUG else

#endif	// DEBUG_H 
