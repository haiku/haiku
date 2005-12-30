/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <platform.h>

#include <boot/kernel_args.h>
#include <platform/openfirmware/openfirmware.h>

status_t
platform_init(struct kernel_args *kernelArgs)
{
	return of_init(
		(int(*)(void*))kernelArgs->platform_args.openfirmware_entry);
}
