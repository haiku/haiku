/*
 * Copyright 2008, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_ARCH_SPARC_ASM_DEFS_H
#define SYSTEM_ARCH_SPARC_ASM_DEFS_H


/* include arch version helpers */
//#include <arch_sparc_version.h>

#define SYMBOL(name)			.global name; name
#define SYMBOL_END(name)		1: .size name, 1b - name
#define STATIC_FUNCTION(name)	.type name, %function; name
#define FUNCTION(name)			.global name; .type name, %function; name
#define FUNCTION_END(name)		1: .size name, 1b - name

#endif	/* SYSTEM_ARCH_SPARC_ASM_DEFS_H */


