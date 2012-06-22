/*
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


#define CHATTY 0

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
	// there are no rel entries in PPC elf
	return B_NO_ERROR;
}


static inline void
write_word32(addr_t P, Elf32_Word value)
{
	*(Elf32_Word*)P = value;
}


static inline void
write_word30(addr_t P, Elf32_Word value)
{
	// bits 0:29
	*(Elf32_Word*)P = (*(Elf32_Word*)P & 0x3) | (value << 2);
}


static inline bool
write_low24_check(addr_t P, Elf32_Word value)
{
	// bits 6:29
	if ((value & 0x3f000000) && (~value & 0x3f800000))
		return false;
	*(Elf32_Word*)P = (*(Elf32_Word*)P & 0xfc000003)
		| ((value & 0x00ffffff) << 2);
	return true;
}


static inline bool
write_low14_check(addr_t P, Elf32_Word value)
{
	// bits 16:29
	if ((value & 0x3fffc000) && (~value & 0x3fffe000))
		return false;
	*(Elf32_Word*)P = (*(Elf32_Word*)P & 0xffff0003)
		| ((value & 0x00003fff) << 2);
	return true;
}


static inline void
write_half16(addr_t P, Elf32_Word value)
{
	// bits 16:29
	*(Elf32_Half*)P = (Elf32_Half)value;
}


static inline bool
write_half16_check(addr_t P, Elf32_Word value)
{
	// bits 16:29
	if ((value & 0xffff0000) && (~value & 0xffff8000))
		return false;
	*(Elf32_Half*)P = (Elf32_Half)value;
	return true;
}


static inline Elf32_Word
lo(Elf32_Word value)
{
	return (value & 0xffff);
}


static inline Elf32_Word
hi(Elf32_Word value)
{
	return ((value >> 16) & 0xffff);
}


static inline Elf32_Word
ha(Elf32_Word value)
{
	return (((value >> 16) + (value & 0x8000 ? 1 : 0)) & 0xffff);
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
			case R_PPC_SECTOFF:
			case R_PPC_SECTOFF_LO:
			case R_PPC_SECTOFF_HI:
			case R_PPC_SECTOFF_HA:
				dprintf("arch_elf_relocate_rela(): Getting section relative "
					"symbol addresses not yet supported!\n");
				return B_ERROR;

			case R_PPC_ADDR32:
			case R_PPC_ADDR24:
			case R_PPC_ADDR16:
			case R_PPC_ADDR16_LO:
			case R_PPC_ADDR16_HI:
			case R_PPC_ADDR16_HA:
			case R_PPC_ADDR14:
			case R_PPC_ADDR14_BRTAKEN:
			case R_PPC_ADDR14_BRNTAKEN:
			case R_PPC_REL24:
			case R_PPC_REL14:
			case R_PPC_REL14_BRTAKEN:
			case R_PPC_REL14_BRNTAKEN:
			case R_PPC_GLOB_DAT:
			case R_PPC_UADDR32:
			case R_PPC_UADDR16:
			case R_PPC_REL32:
			case R_PPC_SDAREL16:
			case R_PPC_ADDR30:
			case R_PPC_JMP_SLOT:
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
			case R_PPC_NONE:
				break;

			case R_PPC_COPY:
				// TODO: Implement!
				dprintf("arch_elf_relocate_rela(): R_PPC_COPY not yet "
					"supported!\n");
				return B_ERROR;

			case R_PPC_ADDR32:
			case R_PPC_GLOB_DAT:
			case R_PPC_UADDR32:
				write_word32(P, S + A);
				break;

			case R_PPC_ADDR24:
				if (write_low24_check(P, (S + A) >> 2))
					break;
dprintf("R_PPC_ADDR24 overflow\n");
				return B_BAD_DATA;

			case R_PPC_ADDR16:
			case R_PPC_UADDR16:
				if (write_half16_check(P, S + A))
					break;
dprintf("R_PPC_ADDR16 overflow\n");
				return B_BAD_DATA;

			case R_PPC_ADDR16_LO:
				write_half16(P, lo(S + A));
				break;

			case R_PPC_ADDR16_HI:
				write_half16(P, hi(S + A));
				break;

			case R_PPC_ADDR16_HA:
				write_half16(P, ha(S + A));
				break;

			case R_PPC_ADDR14:
			case R_PPC_ADDR14_BRTAKEN:
			case R_PPC_ADDR14_BRNTAKEN:
				if (write_low14_check(P, (S + A) >> 2))
					break;
dprintf("R_PPC_ADDR14 overflow\n");
				return B_BAD_DATA;

			case R_PPC_REL24:
				if (write_low24_check(P, (S + A - P) >> 2))
					break;
dprintf("R_PPC_REL24 overflow: 0x%lx\n", (S + A - P) >> 2);
				return B_BAD_DATA;

			case R_PPC_REL14:
			case R_PPC_REL14_BRTAKEN:
			case R_PPC_REL14_BRNTAKEN:
				if (write_low14_check(P, (S + A - P) >> 2))
					break;
dprintf("R_PPC_REL14 overflow\n");
				return B_BAD_DATA;

			case R_PPC_GOT16:
				REQUIRE_GOT;
				if (write_half16_check(P, G + A))
					break;
dprintf("R_PPC_GOT16 overflow\n");
				return B_BAD_DATA;

			case R_PPC_GOT16_LO:
				REQUIRE_GOT;
				write_half16(P, lo(G + A));
				break;

			case R_PPC_GOT16_HI:
				REQUIRE_GOT;
				write_half16(P, hi(G + A));
				break;

			case R_PPC_GOT16_HA:
				REQUIRE_GOT;
				write_half16(P, ha(G + A));
				break;

			case R_PPC_JMP_SLOT:
			{
				// If the relative offset is small enough, we fabricate a
				// relative branch instruction ("b <addr>").
				addr_t jumpOffset = S - P;
				if ((jumpOffset & 0xfc000000) != 0
					&& (~jumpOffset & 0xfe000000) != 0) {
					// Offset > 24 bit.
					// TODO: Implement!
					// See System V PPC ABI supplement, p. 5-6!
					dprintf("arch_elf_relocate_rela(): R_PPC_JMP_SLOT: "
						"Offsets > 24 bit currently not supported!\n");
dprintf("jumpOffset: %p\n", (void*)jumpOffset);
					return B_ERROR;
				} else {
					// Offset <= 24 bit
					// 0:5 opcode (= 18), 6:29 address, 30 AA, 31 LK
					// "b" instruction: opcode = 18, AA = 0, LK = 0
					// address: 24 high-order bits of 26 bit offset
					*(uint32*)P = 0x48000000 | ((jumpOffset) & 0x03fffffc);
				}
				break;
			}

			case R_PPC_RELATIVE:
				write_word32(P, B + A);
				break;

			case R_PPC_LOCAL24PC:
// TODO: Implement!
// low24*
// 				if (write_low24_check(P, ?)
// 					break;
// 				return B_BAD_DATA;
				dprintf("arch_elf_relocate_rela(): R_PPC_LOCAL24PC not yet "
					"supported!\n");
				return B_ERROR;

			case R_PPC_REL32:
				write_word32(P, S + A - P);
				break;

			case R_PPC_PLTREL24:
				REQUIRE_PLT;
				if (write_low24_check(P, (L + A - P) >> 2))
					break;
dprintf("R_PPC_PLTREL24 overflow\n");
				return B_BAD_DATA;

			case R_PPC_PLT32:
				REQUIRE_PLT;
				write_word32(P, L + A);
				break;

			case R_PPC_PLTREL32:
				REQUIRE_PLT;
				write_word32(P, L + A - P);
				break;

			case R_PPC_PLT16_LO:
				REQUIRE_PLT;
				write_half16(P, lo(L + A));
				break;

			case R_PPC_PLT16_HI:
				REQUIRE_PLT;
				write_half16(P, hi(L + A));
				break;

			case R_PPC_PLT16_HA:
				write_half16(P, ha(L + A));
				break;

			case R_PPC_SDAREL16:
// TODO: Implement!
// 				if (write_half16_check(P, S + A - _SDA_BASE_))
// 					break;
// 				return B_BAD_DATA;
				dprintf("arch_elf_relocate_rela(): R_PPC_SDAREL16 not yet "
					"supported!\n");
				return B_ERROR;

			case R_PPC_SECTOFF:
				if (write_half16_check(P, R + A))
					break;
dprintf("R_PPC_SECTOFF overflow\n");
				return B_BAD_DATA;

			case R_PPC_SECTOFF_LO:
				write_half16(P, lo(R + A));
				break;

			case R_PPC_SECTOFF_HI:
				write_half16(P, hi(R + A));
				break;

			case R_PPC_SECTOFF_HA:
				write_half16(P, ha(R + A));
				break;

			case R_PPC_ADDR30:
				write_word30(P, (S + A - P) >> 2);
				break;

			default:
				dprintf("arch_elf_relocate_rela: unhandled relocation type %d\n", ELF32_R_TYPE(rel[i].r_info));
				return B_ERROR;
		}
	}

	return B_NO_ERROR;
}

