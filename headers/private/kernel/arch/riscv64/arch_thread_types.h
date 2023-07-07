/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_RISCV64_THREAD_TYPES_H
#define KERNEL_ARCH_RISCV64_THREAD_TYPES_H

#include <kernel.h>


namespace BKernel {
	struct Thread;
}


struct iframe {
	uint64 status;
	uint64 cause;
	uint64 tval;

	uint64 ra __attribute__((aligned (16)));
	uint64 t6;
	uint64 sp;
	uint64 gp;
	uint64 tp;
	uint64 t0;
	uint64 t1;
	uint64 t2;
	uint64 t5;
	uint64 s1;
	uint64 a0;
	uint64 a1;
	uint64 a2;
	uint64 a3;
	uint64 a4;
	uint64 a5;
	uint64 a6;
	uint64 a7;
	uint64 s2;
	uint64 s3;
	uint64 s4;
	uint64 s5;
	uint64 s6;
	uint64 s7;
	uint64 s8;
	uint64 s9;
	uint64 s10;
	uint64 s11;
	uint64 t3;
	uint64 t4;
	uint64 fp;
	uint64 epc;
};


struct arch_context {
	uint64 ra;    //  0
	uint64 s[12]; // 12
	uint64 sp;    // 13
};

struct fpu_context {
	double f[32];
	uint64 fcsr;
};

struct __attribute__((aligned(16))) arch_stack {
	BKernel::Thread* thread;
};

struct arch_thread {
	arch_context context;
	fpu_context fpuContext;
	iframe* userFrame;
	uint64 oldA0;
};

struct arch_team {
	// gcc treats empty structures as zero-length in C, but as if they contain
	// a char in C++. So we have to put a dummy in to be able to use the struct
	// from both in a consistent way.
	char	dummy;
};

struct arch_fork_arg {
	iframe frame;
};


#ifdef __cplusplus
extern "C" {
#endif

void arch_context_switch(arch_context* from, arch_context* to);
void save_fpu(fpu_context* ctx);
void restore_fpu(fpu_context* ctx);
void arch_thread_entry();
void arch_load_user_iframe(arch_stack* stackHeader, iframe* frame)
	__attribute__ ((noreturn));

#ifdef __cplusplus
}
#endif


#endif	/* KERNEL_ARCH_RISCV64_THREAD_TYPES_H */
