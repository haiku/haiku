/*
 * Copyright 2003-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "DebugSupport.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>


/*!
	\file Debug.cpp
	\brief Defines debug output function with printf() signature printing
		   into a file.

	\note The initialization is not thread safe!
*/


// locking support
static int32 init_counter = 0;
static sem_id dbg_printf_sem = -1;
static thread_id dbg_printf_thread = -1;
static int dbg_printf_nesting = 0;


#if DEBUG_PRINT
static int out = -1;
#endif


status_t
init_debugging()
{
	status_t error = B_OK;
	if (init_counter++ == 0) {
		// open the file
		#if DEBUG_PRINT
			out = open(DEBUG_PRINT_FILE, O_RDWR | O_CREAT | O_TRUNC);
			if (out < 0) {
				error = errno;
				init_counter--;
			}
		#endif	// DEBUG_PRINT
		// allocate the semaphore
		if (error == B_OK) {
			dbg_printf_sem = create_sem(1, "dbg_printf");
			if (dbg_printf_sem < 0)
				error = dbg_printf_sem;
		}
		if (error == B_OK) {
			#if DEBUG
				__out("##################################################\n");
			#endif
		} else
			exit_debugging();
	}
	return error;
}


status_t
exit_debugging()
{
	status_t error = B_OK;
	if (--init_counter == 0) {
		#if DEBUG_PRINT
			close(out);
			out = -1;
		#endif	// DEBUG_PRINT
		delete_sem(dbg_printf_sem);
	} else
		error = B_NO_INIT;
	return error;
}


static inline bool
dbg_printf_lock()
{
	thread_id thread = find_thread(NULL);
	if (thread != dbg_printf_thread) {
		if (acquire_sem(dbg_printf_sem) != B_OK)
			return false;
		dbg_printf_thread = thread;
	}
	dbg_printf_nesting++;
	return true;
}


static inline void
dbg_printf_unlock()
{
	thread_id thread = find_thread(NULL);
	if (thread != dbg_printf_thread)
		return;
	dbg_printf_nesting--;
	if (dbg_printf_nesting == 0) {
		dbg_printf_thread = -1;
		release_sem(dbg_printf_sem);
	}
}


void
dbg_printf_begin()
{
	dbg_printf_lock();
}


void
dbg_printf_end()
{
	dbg_printf_unlock();
}


#if DEBUG_PRINT

void
dbg_printf(const char *format,...)
{
	if (!dbg_printf_lock())
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
	write(out, buffer, strlen(buffer));
	dbg_printf_unlock();
}

#endif	// DEBUG_PRINT
