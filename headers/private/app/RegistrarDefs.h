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
#include <Roster.h>

// names
extern const char *kRegistrarSignature;
extern const char *kRosterThreadName;
extern const char *kRosterPortName;
extern const char *kRAppLooperPortName;

// message constants
enum {
	// replies
	B_REG_SUCCESS					= 'rgsu',
	B_REG_ERROR						= 'rger',
	// general requests
	B_REG_GET_MIME_MESSENGER		= 'rgmm',
	B_REG_GET_CLIPBOARD_MESSENGER	= 'rgcm',
	// roster requests
	B_REG_ADD_APP					= 'rgaa',
	B_REG_COMPLETE_REGISTRATION		= 'rgcr',
	B_REG_IS_PRE_REGISTERED			= 'rgip',
	B_REG_REMOVE_PRE_REGISTERED_APP	= 'rgrp',
	B_REG_REMOVE_APP				= 'rgra',
	B_REG_SET_THREAD_AND_TEAM		= 'rgtt',
	B_REG_GET_RUNNING_APP_INFO		= 'rgri',
};

// type constants
enum {
	B_REG_APP_INFO_TYPE				= 'rgai',	// app_info
};

// error constants
#define B_REGISTRAR_ERROR_BASE		(B_ERRORS_END + 1)

enum {
	B_REG_ALREADY_REGISTERED		= B_REGISTRAR_ERROR_BASE,
		// A team tries to register a second time.
	B_REG_APP_NOT_REGISTERED,
	B_REG_APP_NOT_PRE_REGISTERED,
};

// misc constants
enum {
	B_REG_DEFAULT_APP_FLAGS			= B_MULTIPLE_LAUNCH | B_ARGV_ONLY
									  | _B_APP_INFO_RESERVED1_,
	B_REG_APP_LOOPER_PORT_CAPACITY	= 100,
};

// structs

// a flat app_info -- to be found in B_REG_APP_INFO_TYPE message fields
struct flat_app_info {
	app_info	info;
	char		ref_name[B_FILE_NAME_LENGTH + 1];
};

#endif	// REGISTRAR_DEFS_H
