/*
 * Copyright 2019-2020, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Copyright 2010, Ithamar R. Adema <ithamar.adema@team-embedded.nl>
 * Copyright 2009, Johannes Wischert, johanneswi@gmail.com.
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <elf_priv.h>
#include <boot/elf.h>
#include <arch/elf.h>


//#define TRACE_ARCH_ELF
#ifdef TRACE_ARCH_ELF
#	define TRACE(x) dprintf x
#	define CHATTY 1
#else
#	define TRACE(x) ;
#	define CHATTY 0
#endif


#ifndef _BOOT_MODE
static bool
is_in_image(struct elf_image_info *image, addr_t address)
{
	return (address >= image->text_region.start
			&& address < image->text_region.start + image->text_region.size)
		|| (address >= image->data_region.start
			&& address < image->data_region.start + image->data_region.size);
}
#endif	// !_BOOT_MODE


#ifdef _BOOT_MODE
status_t
boot_arch_elf_relocate_rel(struct preloaded_elf64_image *image, Elf64_Rel *rel,
	int rel_len)
#else
int
arch_elf_relocate_rel(struct elf_image_info *image,
	struct elf_image_info *resolve_image, Elf64_Rel *rel, int rel_len)
#endif
{
	// there are no rel entries in M68K elf
	return B_NO_ERROR;
}


static inline void
write_word32(addr_t P, Elf64_Word value)
{
	*(Elf64_Word*)P = value;
}


static inline void
write_word64(addr_t P, Elf64_Xword value)
{
	*(Elf64_Xword*)P = value;
}


static inline void
write_hi30(addr_t P, Elf64_Word value)
{
	*(Elf64_Word*)P |= value >> 2;
}


static inline void
write_hi22(addr_t P, Elf64_Word value)
{
	*(Elf64_Word*)P |= value >> 10;
}


static inline void
write_lo10(addr_t P, Elf64_Word value)
{
	*(Elf64_Word*)P |= value & 0x3ff;
}


#ifdef _BOOT_MODE
status_t
boot_arch_elf_relocate_rela(struct preloaded_elf64_image *image,
	Elf64_Rela *rel, int rel_len)
#else
int
arch_elf_relocate_rela(struct elf_image_info *image,
	struct elf_image_info *resolve_image, Elf64_Rela *rel, int rel_len)
#endif
{
	int i;
	Elf64_Sym *sym;
	int vlErr;

	Elf64_Addr S = 0;	// symbol address
	//addr_t R = 0;		// section relative symbol address

	//addr_t G = 0;		// GOT address
	//addr_t L = 0;		// PLT address

	#define P	((addr_t)(image->text_region.delta + rel[i].r_offset))
	#define A	((addr_t)rel[i].r_addend)
	#define B	(image->text_region.delta)

	// TODO: Get the GOT address!
	#define REQUIRE_GOT	\
		if (G == 0) {	\
			dprintf("arch_elf_relocate_rela(): Failed to get GOT address!\n"); \
			return B_ERROR;	\
		}

	// TODO: Get the PLT address!
	#define REQUIRE_PLT	\
		if (L == 0) {	\
			dprintf("arch_elf_relocate_rela(): Failed to get PLT address!\n"); \
			return B_ERROR;	\
		}

	for (i = 0; i * (int)sizeof(Elf64_Rela) < rel_len; i++) {
#if CHATTY
		dprintf("looking at rel type %" PRIu64 ", offset 0x%lx, sym 0x%lx, "
			"addend 0x%lx\n", ELF64_R_TYPE(rel[i].r_info), rel[i].r_offset,
			ELF64_R_SYM(rel[i].r_info), rel[i].r_addend);
#endif
		// Relocation types and what to do with them are defined in Oracle docs
		// Documentation Home > Linker and Libraries Guide
		// 	> Chapter 7 Object File Format > File Format > Relocation Sections
		// 	> Relocation Types (Processor-Specific) > SPARC: Relocation Types
		// https://docs.oracle.com/cd/E19120-01/open.solaris/819-0690/chapter6-24/index.html
		// https://docs.oracle.com/cd/E19120-01/open.solaris/819-0690/chapter6-24-1/index.html
		switch (ELF64_R_TYPE(rel[i].r_info)) {
			case R_SPARC_WDISP30:
			case R_SPARC_HI22:
			case R_SPARC_LO10:
			case R_SPARC_GLOB_DAT:
			case R_SPARC_JMP_SLOT:
			case R_SPARC_64:
				sym = SYMBOL(image, ELF64_R_SYM(rel[i].r_info));
#ifdef _BOOT_MODE
				vlErr = boot_elf_resolve_symbol(image, sym, &S);
#else
				vlErr = elf_resolve_symbol(image, sym, resolve_image, &S);
#endif
				if (vlErr < 0) {
					dprintf("%s(): Failed to relocate "
						"entry index %d, rel type %" PRIu64 ", offset 0x%lx, "
						"sym 0x%lx, addend 0x%lx\n", __FUNCTION__, i,
						ELF64_R_TYPE(rel[i].r_info),
						rel[i].r_offset, ELF64_R_SYM(rel[i].r_info),
						rel[i].r_addend);
					return vlErr;
				}
				break;
		}

		switch (ELF64_R_TYPE(rel[i].r_info)) {
			case R_SPARC_WDISP30:
			{
				write_hi30(P, S + A - P);
			}
			case R_SPARC_HI22:
			{
				write_hi22(P, S + A);
				break;
			}
			case R_SPARC_LO10:
			{
				write_lo10(P, S + A);
				break;
			}
			case R_SPARC_GLOB_DAT:
			{
				write_word64(P, S + A);
				break;
			}
			case R_SPARC_JMP_SLOT:
			{
				// Created by the link-editor for dynamic objects to provide
				// lazy binding. The relocation offset member gives the
				// location of a procedure linkage table entry. The runtime
				// linker modifies the procedure linkage table entry to
				// transfer control to the designated symbol address.
				addr_t jumpOffset = S - (P + 8);
				if ((jumpOffset & 0xc0000000) != 0
					&& (~jumpOffset & 0xc0000000) != 0) {
					// Offset > 30 bit.
					// TODO: Implement!
					// See https://docs.oracle.com/cd/E26502_01/html/E26507/chapter6-1235.html
					// examples .PLT102 and .PLT103
					dprintf("arch_elf_relocate_rela(): R_SPARC_JMP_SLOT: "
						"Offsets > 30 bit currently not supported!\n");
					dprintf("jumpOffset: %p\n", (void*)jumpOffset);
					return B_ERROR;
				} else {
					uint32* instructions = (uint32*)P;
					// We need to use a call instruction because it has a lot
					// of space for the destination (30 bits). However, it
					// erases o7, which we don't want.
					// We could avoid this with a JMPL if the displacement was
					// small enough, but it probably isn't.
					// So, we store o7 in g1 before the call, and restore it
					// in the branch delay slot. Crazy, but it works!
					instructions[0] = 0x01000000; // NOP to preserve the alignment?
					instructions[1] = 0x8210000f; // MOV %o7, %g1
					instructions[2] = 0x40000000 | ((jumpOffset >> 2) & 0x3fffffff);
					instructions[3] = 0x9e100001; // MOV %g1, %o7
				}
				break;
			}
			case R_SPARC_RELATIVE:
			{
				write_word32(P, B + A);
				break;
			}
			case R_SPARC_64:
			{
				write_word64(P, S + A);
				break;
			}
			default:
				dprintf("arch_elf_relocate_rela: unhandled relocation type %"
					PRIu64 "\n", ELF64_R_TYPE(rel[i].r_info));
				return B_ERROR;
		}
	}
	return B_OK;
}
