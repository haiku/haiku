/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_FAULTS
#define _NEWOS_KERNEL_ARCH_FAULTS

#include <stage2.h>

int faults_init(kernel_args *ka);
int arch_faults_init(kernel_args *ka);

#endif

