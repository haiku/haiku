/*
 * Copyright 2009, Colin Günther. All rights reserved.
 * Copyright 2007, Hugo Santos. All rights reserved.
 * Copyright 2018, Haiku, Inc. All rights reserved.
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
#  error Need a cpufunc.h for this arch!
#endif

#endif /* _FBSD_COMPAT_MACHINE_CPUFUNC_H_ */
