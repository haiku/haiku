/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

//#include <arch_platform.h>

#include <new>

#include <KernelExport.h>
#include <arch/platform.h>
#include <boot/kernel_args.h>
//#include <platform/openfirmware/openfirmware.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>


#if 0
static M68KPlatform *sM68KPlatform;


// constructor
M68KPlatform::M68KPlatform(platform_type platformType,
	m68k_platform_type m68kPlatformType)
	: fPlatformType(platformType),
	fM68KPlatformType(m68kPlatformType)
{
}


// destructor
M68KPlatform::~M68KPlatform()
{
}


// Default
M68KPlatform *
M68KPlatform::Default()
{
	return sM68KPlatform;
}


// # pragma mark -
#endif

status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
	#warning ARM:WRITEME
	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	#warning ARM:WRITEME
	//sM68KPlatform->InitPostVM(kernelArgs);
	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}
