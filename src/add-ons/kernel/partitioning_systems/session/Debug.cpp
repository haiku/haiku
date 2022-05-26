//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//  This version copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Initial version copyright (c) 2002 Axel Dörfler, axeld@pinc-software.de
//----------------------------------------------------------------------

/*! \file Debug.cpp

	Support code for handy debugging macros.
*/

#include "Debug.h"

#include <KernelExport.h>
#include <TLS.h>

#include <string.h>

//----------------------------------------------------------------------
// Long-winded overview of the debug output macros:
//----------------------------------------------------------------------
/*! \def DEBUG_INIT()
	\brief Increases the indentation level, prints out the enclosing function's
	name, and creates a \c DebugHelper object on the stack to automatically
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
/*! \def REPORT_ERROR(error)
	\brief Calls \c LPRINT(x) with a format string listing the error
	code in \c error (assumed to be a \c status_t value) and the
	corresponding text error code returned by a call to \c strerror().
	
	This function is called by the \c RETURN* macros, and isn't really
	intended for general consumption, but you might find it useful.
	
	\param error A \c status_t error code to report.
	
	If DEBUG is undefined, does nothing.
*/
//----------------------------------------------------------------------
/*! \def RETURN_ERROR(error)
	\brief Calls \c REPORT_ERROR(error) if error is a an error code (i.e.
	negative), otherwise remains silent. In either case, the enclosing
	function is then exited with a call to \c "return error;".
		
	\param error A \c status_t error code to report (if negative) and return.
	
	If DEBUG is undefined, silently returns the value in \c error.
*/
//----------------------------------------------------------------------
/*! \def RETURN(error)
	\brief Prints out a description of the error code being returned
	(which, in this case, may be either "erroneous" or "successful")
	and then exits the enclosing function with a call to \c "return error;".
		
	\param error A \c status_t error code to report and return.
	
	If DEBUG is undefined, silently returns the value in \c error.
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
/*! \def DIE(x)
	\brief Drops the user into the appropriate debugger (user or kernel)
	after printing out the handy message bundled in the parenthesee
	enclosed printf-style format string found in \a x.

	\param x A printf-style format string enclosed in an extra set of parenteses,
	         e.g. PRINT(("%d\n", 0));	
*/


//----------------------------------------------------------------------
// declarations
//----------------------------------------------------------------------

static void indent(uint8 tabCount);
static void unindent(uint8 tabCount);
#ifdef USER
	static int32 get_tls_handle();
#endif

//! Used to keep the tls handle from being allocated more than once.
vint32 tls_spinlock = 0;

/*! \brief Used to flag whether the tls handle has been allocated yet.

	Not sure if this really needs to be \c volatile or not...
*/
volatile bool tls_handle_initialized = false;

//! The tls handle of the tls var used to store indentation info.
int32 tls_handle = 0;

//----------------------------------------------------------------------
// public functions
//----------------------------------------------------------------------

/*! \brief Returns the current debug indentation level for the
	current thread.
	
	NOTE: indentation is currently unsupported for R5::kernelland due
	to lack of thread local storage support.
*/
int32
_get_debug_indent_level()
{
#ifdef USER
	return (int32)tls_get(get_tls_handle());
#else
	return 1;
#endif
}
	
//----------------------------------------------------------------------
// static functions
//----------------------------------------------------------------------

/*! \brief Increases the current debug indentation level for
	the current thread by 1.
*/
void
indent(uint8 tabCount)
{
#ifdef USER
	tls_set(get_tls_handle(), (void*)(_get_debug_indent_level()+tabCount));
#endif
}

/*! \brief Decreases the current debug indentation level for
	the current thread by 1.
*/
void
unindent(uint8 tabCount)
{
#ifdef USER
	tls_set(get_tls_handle(), (void*)(_get_debug_indent_level()-tabCount));
#endif
}

#ifdef USER
/*! \brief Returns the thread local storage handle used to store
	indentation information, allocating the handle first if
	necessary.
*/
int32
get_tls_handle()
{
	// Init the tls handle if this is the first call.
	if (!tls_handle_initialized) {
		if (atomic_or(&tls_spinlock, 1) == 0) {
			// First one in gets to init
			tls_handle = tls_allocate();	
			tls_handle_initialized = true;
			atomic_and(&tls_spinlock, 0);
		} else {
			// All others must wait patiently
			while (!tls_handle_initialized) {
				snooze(1);
			}
		}
	}
	return tls_handle;
}
#endif

/*! \brief Helper class for initializing the debugging output
	file.
	
	Note that this hummer isn't threadsafe, but it doesn't really
	matter for our concerns, since the worst it'll result in is
	a dangling file descriptor, and that would be in the case of
	two or more volumes being mounted almost simultaneously...
	not too big of a worry.
*/
class DebugOutputFile {
public:
	DebugOutputFile(const char *filename = NULL) 
		: fFile(-1)
	{
		Init(filename);
	}
	
	~DebugOutputFile() {
		if (fFile >= 0)
			close(fFile);
	}
	
	void Init(const char *filename) {
		if (fFile < 0 && filename)
			fFile = open(filename, O_RDWR | O_CREAT | O_TRUNC);
	}
	
	int File() const { return fFile; }
private:
	int fFile;
};

DebugOutputFile *out = NULL;

/*!	\brief It doesn't appear that the constructor for the global
	\c out variable is called when built as an R5 filesystem add-on,
	so this function needs to be called in udf_mount to let the
	magic happen.
*/
void initialize_debugger(const char *filename)
{
#if DEBUG_TO_FILE
	if (!out) {
		out = new DebugOutputFile(filename);
		dbg_printf("out was NULL!\n");
	} else {
		DebugOutputFile *temp = out;
		out = new DebugOutputFile(filename);
		dbg_printf("out was %p!\n", temp);
	}
#endif
}

// dbg_printf, stolen from Ingo's ReiserFS::Debug.cpp.
void
dbg_printf(const char *format,...)
{
#if DEBUG_TO_FILE
	if (!out)
		return;

	char buffer[1024];
	va_list args;
	va_start(args, format);
	// no vsnprintf() on PPC and in kernel
	#if defined(__i386__) && USER
		vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	#else
		vsprintf(buffer, format, args);
	#endif
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';
	write(out->File(), buffer, strlen(buffer));
#endif
}

//----------------------------------------------------------------------
// DebugHelper
//----------------------------------------------------------------------

/*! \brief Increases the current indentation level.
*/
DebugHelper::DebugHelper(const char *className, uint8 tabCount)
	: fTabCount(tabCount)
	, fClassName(NULL)
{
	indent(fTabCount);
	if (className) {
		fClassName = (char*)malloc(strlen(className)+1);
		if (fClassName)
			strcpy(fClassName, className);
	}
}

/*! \brief Decreases the current indentation level.
*/
DebugHelper::~DebugHelper()
{
	unindent(fTabCount);
	free(fClassName);
}

