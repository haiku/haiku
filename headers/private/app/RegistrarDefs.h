//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RegistrarDefs.h
//	Author(s):		Ingo Weinhold (bonefish@users.sf.net)
//	Description:	API classes - registrar interface.
//------------------------------------------------------------------------------

#ifndef REGISTRAR_DEFS_H
#define REGISTRAR_DEFS_H

#include <Errors.h>

// names
extern const char *kRegistrarSignature;
extern const char *kRosterThreadName;
extern const char *kRosterPortName;

// message constants
enum {
	B_REG_SUCCESS					= 'rgsu',
	B_REG_ERROR						= 'rger',
	B_REG_GET_MIME_MESSENGER		= 'rgmm',
	B_REG_GET_CLIPBOARD_MESSENGER	= 'rgcm',
};

// type constants
enum {
	B_REG_APP_INFO_TYPE				= 'rgai',	// app_info
};

// error constants
#define B_REGISTRAR_ERROR_BASE		(B_ERRORS_END + 1)

enum {
	B_REG_ALREADY_REGISTERED		= B_REGISTRAR_ERROR_BASE,
};

#endif	// REGISTRAR_DEFS_H
