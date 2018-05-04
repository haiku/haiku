/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */

/*!	Contains the ELF compat loader */


//#define TRACE_ARCH_ELF
#define ELF32_COMPAT 1


#define arch_elf_relocate_rel arch_elf32_relocate_rel
#define arch_elf_relocate_rela arch_elf32_relocate_rela
#define elf_resolve_symbol elf32_resolve_symbol


#include "arch_elf.cpp"
