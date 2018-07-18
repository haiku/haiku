/*
 * Copyright 2009, Colin Günther. All rights reserved.
 * Copyright 2007, Hugo Santos. All rights reserved.
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _FBSD_COMPAT_MACHINE_BUS_H_
#define _FBSD_COMPAT_MACHINE_BUS_H_

#include <machine/_bus.h>
#include <machine/cpufunc.h>

#if defined(__i386__) || defined(__amd64__)
#  include <machine/x86/bus.h>
#else
#  error Need a bus.h for this arch!
#endif

#endif /* _FBSD_COMPAT_MACHINE_BUS_H_ */
