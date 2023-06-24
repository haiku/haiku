/*
 * Copyright 2023, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_MATH_H
#define _KERNEL_MATH_H

/*
	When building the kernel, the bootloader or kernel add-ons, the Haiku build system passes the
	`-ffreestanding` argument to GCC, in order to make sure that only the C++ language features can
	be used that do not require the C/C++ standard library.
	The Haiku kernel and boot loaders include part of the standard library in the kernel/boot
	loader. It uses the C/C++ standard library headers (like this one) to expose the
	functions/features that are available in the kernel/boot loader.

	If we are building for the kernel or the boot loader, the logic below will tell GCC's C++
	headers to include the underlying posix headers, so that the kernel, the boot loader and kernel
	add-ons can link to the symbols defined in them.

	When we are NOT building for the kernel or the boot loader, we fall back to GCC's default
	behaviour.
*/

#if (defined(_KERNEL_MODE) || defined(_BOOT_MODE)) && !defined(_GLIBCXX_INCLUDE_NEXT_C_HEADERS)
# define _GLIBCXX_INCLUDE_NEXT_C_HEADERS
# include_next <math.h>
# undef _GLIBCXX_INCLUDE_NEXT_C_HEADERS
#else
# include_next <math.h>
#endif


#endif // _KERNEL_MATH_H
