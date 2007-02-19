// Debug.cpp
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
		} else
			dbg_printf("##################################################\n");
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
		}
	} else
		error = B_NO_INIT;
	return error;
}

// dbg_printf
void
dbg_printf(const char *format,...)
{
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
}

#endif	// DEBUG_PRINT
