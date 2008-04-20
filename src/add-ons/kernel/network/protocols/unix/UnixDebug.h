/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_DEBUG_H
#define UNIX_DEBUG_H


#include <Drivers.h>


//#define UNIX_DEBUG_PRINT	dprintf
#define UNIX_DEBUG_PRINT	ktrace_printf

#if UNIX_DEBUG_LEVEL
#	define	TRACE(args...)	UNIX_DEBUG_PRINT(args)
#	define	PRINT_ERROR(error)										\
		do {														\
			UNIX_DEBUG_PRINT("[%ld] l. %d: %s: %s\n",				\
				find_thread(NULL), __LINE__, __PRETTY_FUNCTION__,	\
				strerror(error));									\
		} while (false)
#	if	UNIX_DEBUG_LEVEL >= 2
#		define	REPORT_ERROR(error)	PRINT_ERROR(error)
#		define	RETURN_ERROR(error)									\
			do {													\
				__typeof(error) error_RETURN_ERROR = (error);		\
				PRINT_ERROR(error_RETURN_ERROR);					\
				return error_RETURN_ERROR;							\
			} while (false)
#	else
#		define	REPORT_ERROR(error)									\
			do {													\
				__typeof(error) error_REPORT_ERROR = (error);		\
				if (error_REPORT_ERROR < 0)							\
					PRINT_ERROR(error_REPORT_ERROR);				\
			} while (false)
#		define	RETURN_ERROR(error)									\
			do {													\
				__typeof(error) error_RETURN_ERROR = (error);		\
				if (error_RETURN_ERROR < 0)							\
					PRINT_ERROR(error_RETURN_ERROR);				\
				return error_RETURN_ERROR;							\
			} while (false)
#	endif
#else
#	define	TRACE(args...)	do {} while (false)
#	define	REPORT_ERROR(error)
#	define	RETURN_ERROR(error)	return (error)
#endif


#endif	// UNIX_DEBUG_H
