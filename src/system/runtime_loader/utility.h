/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H

#include <OS.h>


#define	PAGE_MASK (B_PAGE_SIZE - 1)

#define	PAGE_OFFSET(x) ((x) & (PAGE_MASK))
#define	PAGE_BASE(x) ((x) & ~(PAGE_MASK))
#define TO_PAGE_SIZE(x) ((x + (PAGE_MASK)) & ~(PAGE_MASK))


#ifdef __cplusplus
extern "C" {
#endif

void dprintf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif	// UTILITY_H
