/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCH_M68K_VM_H
#define ARCH_M68K_VM_H

#include <vm/VMTranslationMap.h>

/* This many pages will be read/written on I/O if possible */

#define NUM_IO_PAGES	4
	/* 16 kB */

#define PAGE_SHIFT 12

#endif	/* ARCH_M68K_VM_H */
