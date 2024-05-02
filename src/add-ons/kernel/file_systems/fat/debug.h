/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef FAT_DEBUG_H
#define FAT_DEBUG_H


#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#else
#include <stdio.h>

#include <SupportDefs.h>
#endif


#ifdef USER
#define __out printf
#else
#define __out dprintf
#endif

// Which debugger should be used when?
// The DEBUGGER() macro actually has no effect if DEBUG is not defined,
// use the DIE() macro if you really want to die.
#ifdef DEBUG
#ifdef USER
#define DEBUGGER(x) debugger x
#else
#define DEBUGGER(x) kernel_debugger x
#endif
#else
#define DEBUGGER(x) ;
#endif

#ifdef USER
#define DIE(x) debugger x
#else
#define DIE(x) kernel_debugger x
#endif

// Short overview over the debug output macros:
//	PRINT()
//		is for general messages that very unlikely should appear in a release build
//	FATAL()
//		this is for fatal messages, when something has really gone wrong
//	INFORM()
//		general information, as disk size, etc.
//	REPORT_ERROR(status_t)
//		prints out error information
//	RETURN_ERROR(status_t)
//		calls REPORT_ERROR() and return the value
//	D()
//		the statements in D() are only included if DEBUG is defined

#if DEBUG
#define PRINT(...) \
	{ \
		__out("fat[%" B_PRId32 "]: ", find_thread(NULL)); \
		__out(__VA_ARGS__); \
	}
#define REPORT_ERROR(status) \
	__out("fat[%" B_PRId32 "]: %s:%d: %s\n", find_thread(NULL), __FUNCTION__, __LINE__, \
		strerror(status));
#define RETURN_ERROR(err) \
	{ \
		status_t _status = err; \
		if (_status < B_OK) \
			REPORT_ERROR(_status); \
		return _status; \
	}
#define FATAL(x) \
	{ \
		__out("fat[%" B_PRId32 "]: ", find_thread(NULL)); \
		__out x; \
	}
#define INFORM(...) \
	{ \
		__out("fat[%" B_PRId32 "]: ", find_thread(NULL)); \
		__out(__VA_ARGS__); \
	}
#define FUNCTION() __out("fat[%" B_PRId32 "]: %s()\n", find_thread(NULL), __FUNCTION__);
#define FUNCTION_START(...) \
	{ \
		__out("fat[%" B_PRId32 "]: %s() ", find_thread(NULL), __FUNCTION__); \
		__out(__VA_ARGS__); \
	}
#define D(x) \
	{ \
		x; \
	};
#else // !DEBUG
#define PRINT(...) ;
#define REPORT_ERROR(status) \
	__out("fat[%" B_PRId32 "]: %s:%d: %s\n", find_thread(NULL), __FUNCTION__, __LINE__, \
		strerror(status));
#define RETURN_ERROR(err) \
	{ \
		return err; \
	}
#define FATAL(x) \
	{ \
		__out("fat[%" B_PRId32 "]: ", find_thread(NULL)); \
		__out x; \
	}
#define INFORM(...) \
	{ \
		__out("fat[%" B_PRId32 "]: ", find_thread(NULL)); \
		__out(__VA_ARGS__); \
	}
#define FUNCTION() ;
#define FUNCTION_START(...) ;
#define D(x) ;
#endif // !DEBUG

int kprintf_volume(int argc, char** argv);
status_t dprintf_volume(struct mount* bsdVolume);
int kprintf_node(int argc, char** argv);
status_t dprintf_node(struct vnode* bsdNode);
status_t dprintf_winentry(struct msdosfsmount* fatVolume, struct winentry* entry,
	const uint32* index);


#endif // FAT_DEBUG_H
