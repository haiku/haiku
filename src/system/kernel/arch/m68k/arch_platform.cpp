/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <arch_platform.h>

#include <new>

#include <KernelExport.h>

#include <boot/kernel_args.h>
//#include <platform/openfirmware/openfirmware.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>

using BPrivate::M68KAtari;
//using BPrivate::M68KAmiga;
//using BPrivate::M68KMac;
//using BPrivate::M68KNext;


static M68KPlatform *sM68KPlatform;


// constructor
M68KPlatform::M68KPlatform(m68k_platform_type platformType)
	: fPlatformType(platformType)
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


// static buffer for constructing the actual M68KPlatform
static char *sM68KPlatformBuffer[sizeof(M68KAtari)];

status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
#warning M68K: switch platform from kernel args
	// only Atari supported for now
	switch (kernelArgs->arch_args.platform) {
#if 0
		case M68K_PLATFORM_AMIGA:
			sM68KPlatform = new(sM68KPlatformBuffer) M68KAmiga;
			break;
#endif
		case M68K_PLATFORM_ATARI:
			sM68KPlatform = new(sM68KPlatformBuffer) M68KAtari;
			break;
#if 0
		case M68K_PLATFORM_MAC:
			sM68KPlatform = new(sM68KPlatformBuffer) M68KApple;
			break;
		case M68K_PLATFORM_NEXT:
			sM68KPlatform = new(sM68KPlatformBuffer) M68KNext;
			break;
#endif
		default:
			panic("unknown platform d\n", kernelArgs->arch_args.platform);
	}

	return sM68KPlatform->Init(kernelArgs);
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	return sM68KPlatform->InitPostVM(kernelArgs);
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}
