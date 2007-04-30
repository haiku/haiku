/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_COMPATIBILITY_H
#define _FSSH_COMPATIBILITY_H

#ifdef __BEOS__
#	ifndef HAIKU_HOST_PLATFORM_HAIKU
#		include <HaikuBuildCompatibility.h>
#	endif
#else
#	include <BeOSBuildCompatibility.h>
#endif

#endif	// _FSSH_COMPATIBILITY_H

