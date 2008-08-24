/*
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISASM_ARCH_H
#define _DISASM_ARCH_H

#ifdef __cplusplus
extern "C" {
#endif

extern status_t disasm_arch_init();
extern status_t disasm_arch_fini();

extern status_t disasm_arch_dump_insns(addr_t where, int count,
	addr_t baseAddress, int backCount);

#ifdef __cplusplus
}
#endif

#endif /* _DISASM_ARCH_H */
