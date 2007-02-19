// Debug.cpp
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>

#include "Debug.h"

/*!
	\file Debug.cpp
	\brief Defines debug output function with printf() signature printing
		   into a file.

	\note The initialization is not thread safe!
*/

// no need to define it, if not used
#if DEBUG_PRINT

static int32 init_counter = 0;
static int out = -1;
static sem_id dbg_printf_sem = -1;
static thread_id dbg_printf_thread = -1;
static int dbg_printf_nesting = 0;

// init_debugging
status_t
init_debugging()
{
	status_t error = B_OK;
	if (init_counter++ == 0) {
		// open the file
		out = open(DEBUG_PRINT_FILE, O_RDWR | O_CREAT | O_TRUNC);
		if (out < 0) {
			error = errno;
			init_counter--;
		} else {
			dbg_printf("##################################################\n");
			dbg_printf_sem = create_sem(1, "dbg_printf");
		}
	}
	return error;
}

// exit_debugging
status_t
exit_debugging()
{
	status_t error = B_OK;
	if (out >= 0) {
		if (--init_counter == 0) {
			close(out);
			out = -1;
			delete_sem(dbg_printf_sem);
		}
	} else
		error = B_NO_INIT;
	return error;
}

// dbg_printf_lock
static inline
bool
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

// dbg_printf_unlock
static inline
void
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

// dbg_printf
void
dbg_printf(const char *format,...)
{
	if (!dbg_printf_lock())
		return;
	char buffer[1024];
	va_list args;
	va_start(args, format);
	// no vsnprintf() on PPC and in kernel
	#if defined(__INTEL__) && USER
		vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	#else
		vsprintf(buffer, format, args);
	#endif
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';
	write(out, buffer, strlen(buffer));
	dbg_printf_unlock();
}

// dbg_printf_begin
void
dbg_printf_begin()
{
	dbg_printf_lock();
}

// dbg_printf_end
void
dbg_printf_end()
{
	dbg_printf_unlock();
}

#endif	// DEBUG_PRINT
