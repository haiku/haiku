/*
 * Copyright 2004, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef USERLAND_FS_COMPATIBILITY_H
#define USERLAND_FS_COMPATIBILITY_H

#include <BeBuild.h>
#include <Errors.h>


#ifdef HAIKU_TARGET_PLATFORM_BEOS
#	define B_BAD_DATA -2147483632L
#else
#	ifndef closesocket
#		define closesocket(fd)	close(fd)
#	endif
#endif

// a Haiku definition
#ifndef B_BUFFER_OVERFLOW
#	define B_BUFFER_OVERFLOW	EOVERFLOW
#endif

// make Zeta R5 source compatible without needing to link against libzeta.so
#ifdef find_directory
#	undef find_directory
#endif

#endif	// USERLAND_FS_COMPATIBILITY_H
