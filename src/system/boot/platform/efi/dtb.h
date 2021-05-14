/*
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DTB_H
#define DTB_H

#ifndef _ASSEMBLER

#include "efi_platform.h"

#include <util/FixedWidthPointer.h>


extern void dtb_init();
extern void dtb_set_kernel_args();


#endif /* !_ASSEMBLER */

#endif /* DTB_H */
