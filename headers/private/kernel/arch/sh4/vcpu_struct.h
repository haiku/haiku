/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _VCPU_STRUCT_H
#define _VCPU_STRUCT_H

struct vector {
	int (*func)(void *iframe);
};

typedef struct {
	unsigned int *kernel_pgdir;
	unsigned int *user_pgdir;
	unsigned int kernel_asid;
	unsigned int user_asid;
	unsigned int *kstack;
	struct vector vt[256];
} vcpu_struct;

#endif

