/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef POSIX_ERROR_MAPPER_H
#define POSIX_ERROR_MAPPER_H

#include <dlfcn.h>
#include <errno.h>

#include <Errors.h>


#define GET_REAL_FUNCTION(returnValue, function, parameters)		\
	static returnValue (*sReal_##function)parameters				\
		= (returnValue (*)parameters)dlsym(RTLD_DEFAULT, #function)

#define HIDDEN_FUNCTION(function)	asm volatile(".hidden " #function)

#define WRAPPER_FUNCTION(returnValue, function, parameters, body)	\
returnValue function parameters										\
{																	\
	HIDDEN_FUNCTION(function);										\
	GET_REAL_FUNCTION(returnValue, function, parameters);			\
	body															\
}

#endif	// POSIX_ERROR_MAPPER_H
