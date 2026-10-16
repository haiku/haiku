/*
 * Copyright 2018-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _FBSD_COMPAT_MACHINE_CPUFUNC_H_
#define _FBSD_COMPAT_MACHINE_CPUFUNC_H_

#include <sys/cdefs.h>

#if defined(__i386__)
#  include <machine/x86/cpufunc.h>
#elif defined(__x86_64__)
#  include <machine/x86_64/cpufunc.h>
#else
#  include <machine/generic/cpufunc.h>
#endif

#endif /* _FBSD_COMPAT_MACHINE_CPUFUNC_H_ */
