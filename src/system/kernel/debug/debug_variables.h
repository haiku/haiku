/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_VARIABLES_H
#define _KERNEL_DEBUG_VARIABLES_H

#include <SupportDefs.h>


#define MAX_DEBUG_VARIABLE_NAME_LEN 24

#ifdef __cplusplus
extern "C" {
#endif

void	debug_variables_init();

#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _KERNEL_DEBUG_VARIABLES_H
