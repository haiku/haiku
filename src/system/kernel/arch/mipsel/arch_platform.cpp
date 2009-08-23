/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2007 François Revol, revol@free.fr
 * Copyright 2006 Ingo Weinhold, bonefish@cs.tu-berlin.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <arch_platform.h>

#include <new>

#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>


static MipselPlatform *sMipselPlatform;


MipselPlatform::MipselPlatform(mipsel_platform_type platformType)
	:
	fPlatformType(platformType)
{
#warning IMPLEMENT MipselPlatform
}

MipselPlatform::~MipselPlatform()
{
#warning IMPLEMENT ~MipselPlatform
}

MipselPlatform*
MipselPlatform::Default()
{
	return sMipselPlatform;
}


// #pragma mark - Routerboard


namespace BPrivate {

class Routerboard : public MipselPlatform {
public:
							Routerboard();
	virtual					~Routerboard();

	virtual	status_t		Init(struct kernel_args *kernelArgs);
	virtual	status_t		InitSerialDebug(struct kernel_args *kernelArgs);
	virtual	status_t		InitPostVM(struct kernel_args *kernelArgs);
	virtual	status_t		InitRTC(struct kernel_args *kernelArgs,
								struct real_time_data *data);

	virtual	char			SerialDebugGetChar();
	virtual void			SerialDebugPutChar(char c);

	virtual	void			SetHardwareRTC(uint32 seconds);
	virtual	uint32			GetHardwareRTC();

	virtual	void			ShutDown(bool reboot);

private:
			int				fInput;
			int				fOutput;
			int				fRTC;
};

}	// namespace BPrivate

using BPrivate::Routerboard;


Routerboard::Routerboard()
	: MipselPlatform(MIPSEL_PLATFORM_ROUTERBOARD),
	  fInput(-1),
	  fOutput(-1),
	  fRTC(-1)
{
#warning IMPLEMENT Routerboard
}


Routerboard::~Routerboard()
{
#warning IMPLEMENT ~Routerboard
}


status_t
Routerboard::Init(struct kernel_args *kernelArgs)
{
#warning IMPLEMENT Init
	return B_ERROR;
}


status_t
Routerboard::InitSerialDebug(struct kernel_args *kernelArgs)
{
#warning IMPLEMENT InitSerialDebug
	return B_ERROR;
}


status_t
Routerboard::InitPostVM(struct kernel_args *kernelArgs)
{
#warning IMPLEMENT InitPostVM
	return B_ERROR;
}


status_t
Routerboard::InitRTC(struct kernel_args *kernelArgs,
	struct real_time_data *data)
{
#warning IMPLEMENT InitRTC
	return B_ERROR;
}


char
Routerboard::SerialDebugGetChar()
{
#warning IMPLEMENT SerialDebugGetChar
	return 0;
}


void
Routerboard::SerialDebugPutChar(char c)
{
#warning IMPLEMENT SerialDebugPutChar
}


void
Routerboard::SetHardwareRTC(uint32 seconds)
{
#warning IMPLEMENT SetHardwareRTC
}


uint32
Routerboard::GetHardwareRTC()
{
#warning IMPLEMENT GetHardwareRTC
	return 0;
}


void
Routerboard::ShutDown(bool reboot)
{
#warning IMPLEMENT ShutDown
}


// # pragma mark -


// static buffer for constructing the actual MipselPlatform
static char *sMipselPlatformBuffer[sizeof(Routerboard)];


status_t
arch_platform_init(struct kernel_args* kernelArgs)
{
#warning IMPLEMENT arch_platform_init
	return B_ERROR;
}


status_t
arch_platform_init_post_vm(struct kernel_args* kernelArgs)
{
#warning IMPLEMENT arch_platform_init_post_vm
	return B_ERROR;
}


status_t
arch_platform_init_post_thread(struct kernel_args* kernelArgs)
{
#warning IMPLEMENT arch_platform_init_post_thread
	return B_ERROR;
}

