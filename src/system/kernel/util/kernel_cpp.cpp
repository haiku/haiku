/*
 * Copyright 2003-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Ingo Weinhold, bonefish@users.sf.net.
 */

//!	C++ in the kernel


#include "util/kernel_cpp.h"

#ifdef _BOOT_MODE
#	include <boot/platform.h>
#else
#	include <KernelExport.h>
#	include <stdio.h>
#endif

#ifdef _LOADER_MODE
#	define panic printf
#	define dprintf printf
#	define kernel_debugger(x) printf("%s", x)
#endif


// Always define the symbols needed when not linking against libgcc.a --
// we simply override them.

// ... it doesn't seem to work with this symbol at least.
#ifndef USING_LIBGCC
#	if __GNUC__ >= 6 || defined(__clang__)
const std::nothrow_t std::nothrow = std::nothrow_t{ };
#	elif __GNUC__ >= 3
const std::nothrow_t std::nothrow = {};
#	else
const nothrow_t std::nothrow = {};
#	endif
#endif

#if __cplusplus >= 201402L
#define _THROW(x)
#define _NOEXCEPT noexcept
#else
#define _THROW(x) throw (x)
#define _NOEXCEPT throw ()
#endif

const mynothrow_t mynothrow = {};

#if __GNUC__ == 2

extern "C" void
__pure_virtual()
{
	panic("pure virtual function call\n");
}

#elif __GNUC__ >= 3

extern "C" void
__cxa_pure_virtual()
{
	panic("pure virtual function call\n");
}


extern "C" int
__cxa_atexit(void (*hook)(void*), void* data, void* dsoHandle)
{
	return 0;
}


extern "C" void
__cxa_finalize(void* dsoHandle)
{
}

#endif

// full C++ support in the kernel
#if (defined(_KERNEL_MODE) || defined(_LOADER_MODE))
void *
operator new(size_t size) _THROW(std::bad_alloc)
{
	// we don't actually throw any exceptions, but we have to
	// keep the prototype as specified in <new>, or else GCC 3
	// won't like us
	return malloc(size);
}


void *
operator new[](size_t size)
{
	return malloc(size);
}


void *
operator new(size_t size, const std::nothrow_t &) _NOEXCEPT
{
	return malloc(size);
}


void *
operator new[](size_t size, const std::nothrow_t &) _NOEXCEPT
{
	return malloc(size);
}


void *
operator new(size_t size, const mynothrow_t &) _NOEXCEPT
{
	return malloc(size);
}


void *
operator new[](size_t size, const mynothrow_t &) _NOEXCEPT
{
	return malloc(size);
}


void
operator delete(void *ptr) _NOEXCEPT
{
	free(ptr);
}


void
operator delete[](void *ptr) _NOEXCEPT
{
	free(ptr);
}


#if __cplusplus >= 201402L

void
operator delete(void* ptr, std::size_t) _NOEXCEPT
{
	free(ptr);
}


void
operator delete[](void* ptr, std::size_t) _NOEXCEPT
{
	free(ptr);
}

#endif


#ifndef _BOOT_MODE

FILE *stderr = NULL;

extern "C"
int
fprintf(FILE *f, const char *format, ...)
{
	// TODO: Introduce a vdprintf()...
	dprintf("fprintf(`%s',...)\n", format);
	return 0;
}

extern "C"
size_t
fwrite(const void *buffer, size_t size, size_t numItems, FILE *stream)
{
	dprintf("%.*s", int(size * numItems), (char*)buffer);
	return 0;
}

extern "C"
int
fputs(const char *string, FILE *stream)
{
	dprintf("%s", string);
	return 0;
}

extern "C"
int
fputc(int c, FILE *stream)
{
	dprintf("%c", c);
	return 0;
}

#ifndef _LOADER_MODE
extern "C"
int
printf(const char *format, ...)
{
	// TODO: Introduce a vdprintf()...
	dprintf("printf(`%s',...)\n", format);
	return 0;
}
#endif // #ifndef _LOADER_MODE

extern "C"
int
puts(const char *string)
{
	return fputs(string, NULL);
}

#endif	// #ifndef _BOOT_MODE

#if __GNUC__ >= 4

extern "C"
void
_Unwind_DeleteException()
{
	panic("_Unwind_DeleteException");
}

extern "C"
void
_Unwind_Find_FDE()
{
	panic("_Unwind_Find_FDE");
}


extern "C"
void
_Unwind_GetDataRelBase()
{
	panic("_Unwind_GetDataRelBase");
}

extern "C"
void
_Unwind_GetGR()
{
	panic("_Unwind_GetGR");
}

extern "C"
void
_Unwind_GetIP()
{
	panic("_Unwind_GetIP");
}

extern "C"
void
_Unwind_GetIPInfo()
{
	panic("_Unwind_GetIPInfo");
}

extern "C"
void
_Unwind_GetLanguageSpecificData()
{
	panic("_Unwind_GetLanguageSpecificData");
}

extern "C"
void
_Unwind_GetRegionStart()
{
	panic("_Unwind_GetRegionStart");
}

extern "C"
void
_Unwind_GetTextRelBase()
{
	panic("_Unwind_GetTextRelBase");
}

extern "C"
void
_Unwind_RaiseException()
{
	panic("_Unwind_RaiseException");
}

extern "C"
void
_Unwind_Resume()
{
	panic("_Unwind_Resume");
}

extern "C"
void
_Unwind_Resume_or_Rethrow()
{
	panic("_Unwind_Resume_or_Rethrow");
}

extern "C"
void
_Unwind_SetGR()
{
	panic("_Unwind_SetGR");
}

extern "C"
void
_Unwind_SetIP()
{
	panic("_Unwind_SetIP");
}

extern "C"
void
__deregister_frame_info()
{
	panic("__deregister_frame_info");
}

extern "C"
void
__register_frame_info()
{
	panic("__register_frame_info");
}

/* ARM */
extern "C" void
__aeabi_unwind_cpp_pr0(void)
{
	panic("__aeabi_unwind_cpp_pr0");
}

extern "C" void
__aeabi_unwind_cpp_pr1(void)
{
	panic("__aeabi_unwind_cpp_pr1");
}

extern "C" void
__aeabi_unwind_cpp_pr2(void)
{
	panic("__aeabi_unwind_cpp_pr2");
}

extern "C" void
_Unwind_Complete(void)
{
	panic("_Unwind_Complete");
}

extern "C" void
_Unwind_VRS_Set(void)
{
	panic("_Unwind_VRS_Set");
}

extern "C" void
_Unwind_VRS_Get(void)
{
	panic("_Unwind_VRS_Get");
}

extern "C" void
__gnu_unwind_frame(void)
{
	panic("__gnu_unwind_frame");
}

#endif	// __GNUC__ >= 4

extern "C"
void
abort()
{
	while (true)
		panic("abort() called!");
}


#ifndef _BOOT_MODE

extern "C"
void
debugger(const char *message)
{
	kernel_debugger(message);
}

#endif	// #ifndef _BOOT_MODE

#endif	// #if (defined(_KERNEL_MODE) || defined(_LOADER_MODE))


extern "C"
void
exit(int status)
{
	while (true)
		panic("exit() called with status code = %d!", status);
}

