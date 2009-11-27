/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_COMPUTED_ASM_MACROS_H
#define _SYSTEM_COMPUTED_ASM_MACROS_H


#define DEFINE_COMPUTED_ASM_MACRO(macro, value) \
	asm volatile("#define " #macro " %0" : : "i" (value))


#endif	/* _SYSTEM_COMPUTED_ASM_MACROS_H */
