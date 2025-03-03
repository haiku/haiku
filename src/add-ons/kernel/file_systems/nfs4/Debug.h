/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS4_DEBUG_H
#define NFS4_DEBUG_H


#ifdef USER
#define _KERNEL_MODE
	// skip the POSIX dprintf declaration in stdio.h
#include <stdio.h>
#undef _KERNEL_MODE
#endif

#include <DebugSupport.h>


#ifdef DEBUG
#define TRACE(x...) FUNCTION(x)
#define CALLED() FUNCTION_START()
#else
#define TRACE(x...)
#define CALLED()
#endif

#if USER
extern "C" void dprintf(const char *format, ...);
#endif
int kprintf_volume(int argc, char** argv);
int kprintf_inode(int argc, char** argv);


#endif	// NFS4_DEBUG_H
