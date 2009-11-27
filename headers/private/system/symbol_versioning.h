/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SYMBOL_VERSIONING_H
#define _SYSTEM_SYMBOL_VERSIONING_H

#include <BeBuild.h>


#ifdef _KERNEL_MODE
#	define DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION(function, symbol, version) \
		B_DEFINE_SYMBOL_VERSION(function, symbol "KERNEL_" version)
#else
#	define DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION(function, symbol, version) \
		B_DEFINE_SYMBOL_VERSION(function, symbol "LIBROOT_" version)
#endif


#endif	/* _SYSTEM_SYMBOL_VERSIONING_H */
