/*
 * Copyright 2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Core dump support.
 */
#ifndef _KERNEL_CORE_DUMP_H
#define _KERNEL_CORE_DUMP_H


#include <SupportDefs.h>


status_t	core_dump_write_core_file(const char* path, bool killTeam);
void		core_dump_trap_thread();


#endif	// _KERNEL_CORE_DUMP_H
