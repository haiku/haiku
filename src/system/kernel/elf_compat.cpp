/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */

/*!	Contains the ELF compat loader */


//#define TRACE_ELF
#define ELF32_COMPAT 1


#include <elf.h>


#define elf_load_user_image elf32_load_user_image
#define elf_resolve_symbol elf32_resolve_symbol
#define elf_find_symbol elf32_find_symbol
#define elf_parse_dynamic_section elf32_parse_dynamic_section
#define elf_relocate elf32_relocate

#define arch_elf_relocate_rel arch_elf32_relocate_rel
#define arch_elf_relocate_rela arch_elf32_relocate_rela


#include "elf.cpp"
