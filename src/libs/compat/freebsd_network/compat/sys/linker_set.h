/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_LINKER_SET_H_
#define _FBSD_COMPAT_SYS_LINKER_SET_H_


#ifndef _FBSD_COMPAT_SYS_CDEFS_H_
#error this file needs sys/cdefs.h as a prerequisite
#endif

/*
 * The following macros are used to declare global sets of objects, which
 * are collected by the linker into a `linker_set' as defined below.
 * For ELF, this is done by constructing a separate segment for each set.
 */

/*
 * Private macros, not to be used outside this header file.
 */
#ifdef __GNUCLIKE___SECTION
#define __MAKE_SET(set, sym)						\
	static void const * const __set_##set##_sym_##sym 		\
	__section("set_" #set) __used = &sym
#else /* !__GNUCLIKE___SECTION */
#ifndef lint
#error this file needs to be ported to your compiler
#endif /* lint */
#define __MAKE_SET(set, sym)	extern void const * const (__set_##set##_sym_##sym)
#endif /* __GNUCLIKE___SECTION */

/*
 * Public macros.
 */
#define TEXT_SET(set, sym)	__MAKE_SET(set, sym)
#define DATA_SET(set, sym)	__MAKE_SET(set, sym)
#define BSS_SET(set, sym)	__MAKE_SET(set, sym)
#define ABS_SET(set, sym)	__MAKE_SET(set, sym)
#define SET_ENTRY(set, sym)	__MAKE_SET(set, sym)

/*
 * Initialize before referring to a given linker set.
 */
#define SET_DECLARE(set, ptype)						\
	extern ptype *__CONCAT(__start_set_,set);			\
	extern ptype *__CONCAT(__stop_set_,set)

#define SET_BEGIN(set)							\
	(&__CONCAT(__start_set_,set))
#define SET_LIMIT(set)							\
	(&__CONCAT(__stop_set_,set))

/*
 * Iterate over all the elements of a set.
 *
 * Sets always contain addresses of things, and "pvar" points to words
 * containing those addresses.  Thus is must be declared as "type **pvar",
 * and the address of each set item is obtained inside the loop by "*pvar".
 */
#define SET_FOREACH(pvar, set)						\
	for (pvar = SET_BEGIN(set); pvar < SET_LIMIT(set); pvar++)

#endif /* _FBSD_COMPAT_SYS_LINKER_SET_H_ */
