/*
** Copyright 2001-2002, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef _KERNEL_MODULE_H
#define _KERNEL_MODULE_H

#include <drivers/module.h>
#include <kernel.h>
#include <stage2.h>

extern status_t module_init(kernel_args *ka, module_info **sys_module_headers);

#endif	/* _KRENEL_MODULE_H */
