/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_FAULTS_H
#define KERNEL_ARCH_FAULTS_H

struct kernel_args;

int faults_init(kernel_args *ka);
int arch_faults_init(kernel_args *ka);

#endif	/* KERNEL_ARCH_FAULTS_H */
