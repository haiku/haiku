/*
 * Copyright 2018-2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_SIGNAL_H_
#define _ARCH_SIGNAL_H_


/*
 * Architecture-specific structure passed to signal handlers
 */

// TODO: gcc7's RISCV doesn't seem real keen on identifying 32 vs 64 yet.
#if defined(__RISCV64__) || defined(__RISCV__)
struct vregs {
	ulong x[31];
	ulong pc;
	double f[32];
	char fcsr;
};
#endif /* defined(__RISCV64__) */


#endif /* _ARCH_SIGNAL_H_ */
