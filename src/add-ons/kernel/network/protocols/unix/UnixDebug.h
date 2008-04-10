/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_DEBUG_H
#define UNIX_DEBUG_H


#include <Drivers.h>


#if UNIX_DEBUG_LEVEL
#	define	TRACE(args...)	dprintf(args)
#	define	PRINT_ERROR(error)										\
		do {														\
			dprintf("[%ld] l. %d: %s: %s\n", find_thread(NULL),		\
				__LINE__, __PRETTY_FUNCTION__, strerror(error));	\
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
#	define	RETURN_ERROR(error)
#endif


#endif	// UNIX_DEBUG_H
