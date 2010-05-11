/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CONFIG_HAIKU_CONFIG_H
#define _CONFIG_HAIKU_CONFIG_H


/* Determine the architecture and define macros for some fundamental
   properties:
   __HAIKU_ARCH			- short name of the architecture (used in paths)
   __HAIKU_ARCH_<arch>	- defined to 1 for the respective architecture
   __HAIKU_ARCH_64_BIT	- defined to 1 on 64 bit architectures
   __HAIKU_BIG_ENDIAN	- defined to 1 on big endian architectures
*/
#ifdef __INTEL__
#	define __HAIKU_ARCH			x86
#	define __HAIKU_ARCH_X86		1
#elif __x86_64__
#	define __HAIKU_ARCH			x86_64
#	define __HAIKU_ARCH_X86_64	1
#	define __HAIKU_ARCH_64_BIT	1
#elif __POWERPC__
#	define __HAIKU_ARCH				ppc
#	define __HAIKU_ARCH_PPC			1
#	define __HAIKU_BIG_ENDIAN		1
#elif __M68K__
#	define __HAIKU_ARCH				m68k
#	define __HAIKU_ARCH_M68K		1
#	define __HAIKU_BIG_ENDIAN		1
#elif __MIPSEL__
#	define __HAIKU_ARCH				mipsel
#	define __HAIKU_ARCH_MIPSEL		1
#elif __ARM__
#	define __HAIKU_ARCH				arm
#	define __HAIKU_ARCH_ARM			1
#else
#	error Unsupported architecture!
#endif

/* implied properties */
#ifndef __HAIKU_ARCH_64_BIT
#	define	__HAIKU_ARCH_32_BIT		1
#endif
#ifndef __HAIKU_BIG_ENDIAN
#	define	__HAIKU_LITTLE_ENDIAN	1
#endif

/* architecture specific include macros */
#define __HAIKU_ARCH_HEADER(header)					<arch/__HAIKU_ARCH/header>
#define __HAIKU_SUBDIR_ARCH_HEADER(subdir, header)	\
	<subdir/arch/__HAIKU_ARCH/header>

/* BeOS R5 binary compatibility (gcc 2 on x86) */
#if defined(__HAIKU_ARCH_X86) && __GNUC__ == 2
#	define __HAIKU_BEOS_COMPATIBLE	1
#endif

/* BeOS R5 compatible types */
#ifndef __HAIKU_ARCH_64_BIT
/*#ifdef __HAIKU_ARCH_X86*/
	/* TODO: This should be "#ifdef __HAIKU_BEOS_COMPATIBLE", but this will
	   break all gcc 4 C++ optional packages. I.e. switch that at a suitable
	   time.
	*/
#	define __HAIKU_BEOS_COMPATIBLE_TYPES	1
#endif

#endif	/* _CONFIG_HAIKU_CONFIG_H */
