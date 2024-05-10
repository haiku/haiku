/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _NVMM_MACHINE_CPUFUNC_H_
#define _NVMM_MACHINE_CPUFUNC_H_

#if defined(__x86_64__)
#  include <machine/x86_64/cpufunc.h>
#else
#  error Need a cpufunc.h for this arch!
#endif

#endif /* _NVMM_MACHINE_CPUFUNC_H_ */
