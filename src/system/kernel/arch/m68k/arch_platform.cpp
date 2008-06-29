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


status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
	// only Atari supported for now
	switch (kernelArgs->arch_args.platform) {
#if 0
		case M68K_PLATFORM_AMIGA:
			sM68KPlatform = instanciate_m68k_platform_amiga();
			break;
#endif
		case M68K_PLATFORM_ATARI:
			sM68KPlatform = instanciate_m68k_platform_atari();
			break;
#if 0
		case M68K_PLATFORM_MAC:
			sM68KPlatform = instanciate_m68k_platform_mac();
			break;
		case M68K_PLATFORM_NEXT:
			sM68KPlatform = instanciate_m68k_platform_next();
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
