/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "accelerant_protos.h"
#include "accelerant.h"


uint32
vesa_dpms_capabilities(void)
{
	return B_DPMS_ON;
}


uint32
vesa_dpms_mode(void)
{
	return B_DPMS_ON;
}


status_t
vesa_set_dpms_mode(uint32 dpms_flags)
{
	return B_ERROR;
}

