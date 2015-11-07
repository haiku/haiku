#ifndef KERNEL_CPP_H
#define KERNEL_CPP_H
/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/

#ifdef __cplusplus

#include <new>
#include <stdlib.h>

#if _KERNEL_MODE || _LOADER_MODE

using namespace std;
extern const nothrow_t std::nothrow;

// We need new() versions we can use when also linking against libgcc.
// std::nothrow can't be used since it's defined in both libgcc and
// kernel_cpp.cpp.
typedef struct {} mynothrow_t;
extern const mynothrow_t mynothrow;

#endif	// #if _KERNEL_MODE

#endif	// __cplusplus

#endif	/* KERNEL_CPP_H */
