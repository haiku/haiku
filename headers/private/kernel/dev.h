/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_DEV_H
#define _KERNEL_DEV_H

#include <kernel.h>
#include <stage2.h>

int dev_init(kernel_args *ka);
image_id dev_load_dev_module(const char *name, const char *directory);

#endif

