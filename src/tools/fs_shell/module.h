/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_MODULE_PRIVATE_H
#define _FSSH_MODULE_PRIVATE_H

#include "fssh_defs.h"

struct fssh_module_info;

namespace FSShell {

struct kernel_args;

fssh_status_t	module_init(kernel_args *args);
void			register_builtin_module(struct fssh_module_info *info);


}	// namespace FSShell

#endif	// _FSSH_MODULE_PRIVATE_H
