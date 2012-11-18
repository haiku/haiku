/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_SIGNALS_H
#define _KERNEL_ARCH_X86_SIGNALS_H


#include <SupportDefs.h>


void	x86_initialize_commpage_signal_handler();
#ifndef __x86_64__
addr_t	x86_get_user_signal_handler_wrapper(bool beosHandler);
#endif


#endif	// _KERNEL_ARCH_X86_SIGNALS_H
