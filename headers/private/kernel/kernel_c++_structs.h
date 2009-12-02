/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_KERNEL_CPP_STRUCTS_H
#define _KERNEL_KERNEL_CPP_STRUCTS_H


#include <util/kernel_c.h>

#include <kernel_c++_struct_sizes.h>
	// this is the generated defining macros for the structure sizes


// in C++ mode include the headers defining the structures
#ifdef __cplusplus
#	include <condition_variable.h>
#endif

// in C mode define the structures
DEFINE_KERNEL_CPP_STRUCT(ConditionVariable)


#endif	// _KERNEL_KERNEL_CPP_STRUCTS_H
