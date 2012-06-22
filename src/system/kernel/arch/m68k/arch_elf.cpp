/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#ifdef _BOOT_MODE
#include <boot/arch.h>
#endif

#include <KernelExport.h>

#include <elf_priv.h>
#include <arch/elf.h>


//#define TRACE_ARCH_ELF
#ifdef TRACE_ARCH_ELF
#	define TRACE(x) dprintf x
#	define CHATTY 1
#else
#	define TRACE(x) ;
#	define CHATTY 0
#endif


#ifdef TRACE_ARCH_ELF
static const char *kRelocations[] = {
	"R_68K_NONE",
	"R_68K_32",	/* Direct 32 bit  */
	"R_68K_16",	/* Direct 16 bit  */
	"R_68K_8",	/* Direct 8 bit  */
	"R_68K_PC32",	/* PC relative 32 bit */
	"R_68K_PC16",	/* PC relative 16 bit */
	"R_68K_PC8",	/* PC relative 8 bit */
	"R_68K_GOT32",	/* 32 bit PC relative GOT entry */
	"R_68K_GOT16",	/* 16 bit PC relative GOT entry */
	"R_68K_GOT8",	/* 8 bit PC relative GOT entry */
	"R_68K_GOT32O",	/* 32 bit GOT offset */
	"R_68K_GOT16O",	/* 16 bit GOT offset */
	"R_68K_GOT8O",	/* 8 bit GOT offset */
	"R_68K_PLT32",	/* 32 bit PC relative PLT address */
	"R_68K_PLT16",	/* 16 bit PC relative PLT address */
	"R_68K_PLT8",	/* 8 bit PC relative PLT address */
	"R_68K_PLT32O",	/* 32 bit PLT offset */
	"R_68K_PLT16O",	/* 16 bit PLT offset */
	"R_68K_PLT8O",	/* 8 bit PLT offset */
	"R_68K_COPY",	/* Copy symbol at runtime */
	"R_68K_GLOB_DAT",	/* Create GOT entry */
	"R_68K_JMP_SLOT",	/* Create PLT entry */
	"R_68K_RELATIVE",	/* Adjust by program base */
	/* These are GNU extensions to enable C++ vtable garbage collection.  */
	"R_68K_GNU_VTINHERIT",
	"R_68K_GNU_VTENTRY",
#if 0
	"R_386_NONE",
	"R_386_32",			/* add symbol value */
	"R_386_PC32",		/* add PC relative symbol value */
	"R_386_GOT32",		/* add PC relative GOT offset */
	"R_386_PLT32",		/* add PC relative PLT offset */
	"R_386_COPY",		/* copy data from shared object */
	"R_386_GLOB_DAT",	/* set GOT entry to data address */
	"R_386_JMP_SLOT",	/* set GOT entry to code address */
	"R_386_RELATIVE",	/* add load address of shared object */
	"R_386_GOTOFF",		/* add GOT relative symbol address */
	"R_386_GOTPC",		/* add PC relative GOT table address */
#endif
};
#endif

#ifdef _BOOT_MODE
status_t
boot_arch_elf_relocate_rel(struct preloaded_elf32_image *image,
	struct Elf32_Rel *rel, int rel_len)
#else
int
arch_elf_relocate_rel(struct elf_image_info *image,
	struct elf_image_info *resolve_image, struct Elf32_Rel *rel, int rel_len)
#endif
{
	// there are no rel entries in M68K elf
	return B_NO_ERROR;
}


static inline void
write_32(addr_t P, Elf32_Word value)
{
	*(Elf32_Word*)P = value;
}


static inline void
write_16(addr_t P, Elf32_Word value)
{
	// bits 16:29
	*(Elf32_Half*)P = (Elf32_Half)value;
}


static inline bool
write_16_check(addr_t P, Elf32_Word value)
{
	// bits 15:0
	if ((value & 0xffff0000) && (~value & 0xffff8000))
		return false;
	*(Elf32_Half*)P = (Elf32_Half)value;
	return true;
}


static inline bool
write_8(addr_t P, Elf32_Word value)
{
	// bits 7:0
	*(uint8 *)P = (uint8)value;
	return true;
}


static inline bool
write_8_check(addr_t P, Elf32_Word value)
{
	// bits 7:0
	if ((value & 0xffffff00) && (~value & 0xffffff80))
		return false;
	*(uint8 *)P = (uint8)value;
	return true;
}


#ifdef _BOOT_MODE
status_t
boot_arch_elf_relocate_rela(struct preloaded_elf32_image *image,
	struct Elf32_Rela *rel, int rel_len)
#else
int
arch_elf_relocate_rela(struct elf_image_info *image,
	struct elf_image_info *resolve_image, struct Elf32_Rela *rel, int rel_len)
#endif
{
	int i;
	struct Elf32_Sym *sym;
	int vlErr;
	addr_t S = 0;	// symbol address
	addr_t R = 0;	// section relative symbol address

	addr_t G = 0;	// GOT address
	addr_t L = 0;	// PLT address

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

	for (i = 0; i * (int)sizeof(struct Elf32_Rela) < rel_len; i++) {
#if CHATTY
		dprintf("looking at rel type %d, offset 0x%lx, sym 0x%lx, addend 0x%lx\n",
			ELF32_R_TYPE(rel[i].r_info), rel[i].r_offset, ELF32_R_SYM(rel[i].r_info), rel[i].r_addend);
#endif
		switch (ELF32_R_TYPE(rel[i].r_info)) {
			case R_68K_32:
			case R_68K_16:
			case R_68K_8:
			case R_68K_PC32:
			case R_68K_PC16:
			case R_68K_PC8:
			case R_68K_GLOB_DAT:
			case R_68K_JMP_SLOT:
				sym = SYMBOL(image, ELF32_R_SYM(rel[i].r_info));

#ifdef _BOOT_MODE
				vlErr = boot_elf_resolve_symbol(image, sym, &S);
#else
				vlErr = elf_resolve_symbol(image, sym, resolve_image, &S);
#endif
				if (vlErr < 0) {
					dprintf("%s(): Failed to relocate "
						"entry index %d, rel type %d, offset 0x%lx, sym 0x%lx, "
						"addend 0x%lx\n", __FUNCTION__, i, ELF32_R_TYPE(rel[i].r_info),
						rel[i].r_offset, ELF32_R_SYM(rel[i].r_info),
						rel[i].r_addend);
					return vlErr;
				}
				break;
		}

		switch (ELF32_R_TYPE(rel[i].r_info)) {
			case R_68K_NONE:
				break;

			case R_68K_COPY:
				// TODO: Implement!
				dprintf("arch_elf_relocate_rela(): R_68K_COPY not yet "
					"supported!\n");
				return B_ERROR;

			case R_68K_32:
			case R_68K_GLOB_DAT:
				write_32(P, S + A);
				break;

			case R_68K_16:
				if (write_16_check(P, S + A))
					break;
dprintf("R_68K_16 overflow\n");
				return B_BAD_DATA;

			case R_68K_8:
				if (write_8_check(P, S + A))
					break;
dprintf("R_68K_8 overflow\n");
				return B_BAD_DATA;

			case R_68K_PC32:
				write_32(P, (S + A - P));
				break;

			case R_68K_PC16:
				if (write_16_check(P, (S + A - P)))
					break;
dprintf("R_68K_PC16 overflow\n");
				return B_BAD_DATA;

			case R_68K_PC8:
				if (write_8(P, (S + A - P)))
					break;
dprintf("R_68K_PC8 overflow\n");
				return B_BAD_DATA;

			case R_68K_GOT32:
				REQUIRE_GOT;
				write_32(P, (G + A - P));
				break;

			case R_68K_GOT16:
				REQUIRE_GOT;
				if (write_16_check(P, (G + A - P)))
					break;
dprintf("R_68K_GOT16 overflow\n");
				return B_BAD_DATA;

			case R_68K_GOT8:
				REQUIRE_GOT;
				if (write_8_check(P, (G + A - P)))
					break;
dprintf("R_68K_GOT8 overflow\n");
				return B_BAD_DATA;

			case R_68K_GOT32O:
				REQUIRE_GOT;
				write_32(P, (G + A));
				break;

			case R_68K_GOT16O:
				REQUIRE_GOT;
				if (write_16_check(P, (G + A)))
					break;
dprintf("R_68K_GOT16 overflow\n");
				return B_BAD_DATA;

			case R_68K_GOT8O:
				REQUIRE_GOT;
				if (write_8_check(P, (G + A)))
					break;
dprintf("R_68K_GOT8 overflow\n");
				return B_BAD_DATA;

			case R_68K_JMP_SLOT:
				write_32(P, (G + A));
				break;

			case R_68K_RELATIVE:
				write_32(P, B + A);
				break;

			case R_68K_PLT32:
				REQUIRE_PLT;
				write_32(P, (L + A - P));
				break;

			case R_68K_PLT16:
				REQUIRE_PLT;
				if (write_16_check(P, (L + A - P)))
					break;
dprintf("R_68K_PLT16 overflow\n");
				return B_BAD_DATA;

			case R_68K_PLT8:
				REQUIRE_PLT;
				if (write_8_check(P, (L + A - P)))
					break;
dprintf("R_68K_PLT8 overflow\n");
				return B_BAD_DATA;

			case R_68K_PLT32O:
				REQUIRE_PLT;
				write_32(P, (L + A));
				break;

			case R_68K_PLT16O:
				REQUIRE_PLT;
				if (write_16_check(P, (L + A)))
					break;
dprintf("R_68K_PLT16O overflow\n");
				return B_BAD_DATA;

			case R_68K_PLT8O:
				REQUIRE_PLT;
				if (write_8_check(P, (L + A)))
					break;
dprintf("R_68K_PLT8O overflow\n");
				return B_BAD_DATA;

			default:
				dprintf("arch_elf_relocate_rela: unhandled relocation type %d\n", ELF32_R_TYPE(rel[i].r_info));
				return B_ERROR;
		}
	}

	return B_NO_ERROR;
}



