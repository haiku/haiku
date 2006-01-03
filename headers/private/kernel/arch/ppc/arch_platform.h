/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_PPC_ARCH_PLATFORM_H
#define _KERNEL_PPC_ARCH_PLATFORM_H

#include <arch/platform.h>

namespace BPrivate {

class PPCPlatform {
public:
	PPCPlatform();
	virtual ~PPCPlatform();

	static PPCPlatform *Default();

	virtual status_t Init(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitPostVM(struct kernel_args *kernelArgs) = 0;

	virtual char SerialDebugGetChar() = 0;
	virtual void SerialDebugPutChar(char c) = 0;
};

}	// namespace BPrivate

using BPrivate::PPCPlatform;


#endif	// _KERNEL_PPC_ARCH_PLATFORM_H
