/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BE_BUILD_H
#define _BE_BUILD_H


#define B_BEOS_VERSION_4	0x0400
#define B_BEOS_VERSION_4_5	0x0450
#define B_BEOS_VERSION_5	0x0500

#define B_BEOS_VERSION		B_BEOS_VERSION_5
#define B_BEOS_VERSION_MAUI	B_BEOS_VERSION_5

// Haiku (API) version
#define B_HAIKU_VERSION_1_ALPHA_1	0x0010
#define B_HAIKU_VERSION_1			0x0100

#define B_HAIKU_VERSION				B_HAIKU_VERSION_1_ALPHA_1

// Haiku ABI
#define B_HAIKU_ABI_GCC_2			0x01
#define B_HAIKU_ABI_GCC_4			0x02

#if __GNUC__ == 2
#	define B_HAIKU_ABI				B_HAIKU_ABI_GCC_2
#elif __GNUC__ == 4
#	define B_HAIKU_ABI				B_HAIKU_ABI_GCC_4
#else
#	error Unsupported gcc version!
#endif


#define _UNUSED(argument) argument
#define _PACKED __attribute__((packed))
#define _PRINTFLIKE(_format_, _args_) \
	__attribute__((format(__printf__, _format_, _args_)))
#define _EXPORT
#define _IMPORT

#endif	/* _BE_BUILD_H */
