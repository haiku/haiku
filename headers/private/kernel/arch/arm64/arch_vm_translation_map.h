/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_VM_TRANSLATION_MAP_H_
#define _KERNEL_ARCH_ARM64_ARCH_VM_TRANSLATION_MAP_H_

// The base address of TTBR*_EL1 is in bits [47:1] of the register, and the
// low bit is implicitly zero.
static constexpr uint64_t kTtbrBasePhysAddrMask = (((1UL << 47) - 1) << 1);

void arch_vm_install_empty_table_ttbr0(void);

#endif /* _KERNEL_ARCH_ARM64_ARCH_VM_TRANSLATION_MAP_H_ */
