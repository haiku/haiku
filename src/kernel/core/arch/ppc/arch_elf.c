/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <KernelExport.h>

#include <elf_priv.h>
#include <arch/elf.h>


#define CHATTY 0


int 
arch_elf_relocate_rel(struct elf_image_info *image, const char *sym_prepend,
	struct elf_image_info *resolve_image, struct Elf32_Rel *rel, int rel_len)
{
	// there are no rel entries in PPC elf
	return B_NO_ERROR;
}


int 
arch_elf_relocate_rela(struct elf_image_info *image, const char *sym_prepend,
	struct elf_image_info *resolve_image, struct Elf32_Rela *rel, int rel_len)
{
	int i;
	struct Elf32_Sym *sym;
	int vlErr;
	addr_t S = 0;
	addr_t final_val;

#define P         ((addr_t)(image->text_region.delta + rel[i].r_offset))
#define A         ((addr_t)rel[i].r_addend)
#define B         (image->text_region.delta)

	for (i = 0; i * (int)sizeof(struct Elf32_Rela) < rel_len; i++) {
#if CHATTY
		dprintf("looking at rel type %d, offset 0x%x, sym 0x%x, addend 0x%x\n",
			ELF32_R_TYPE(rel[i].r_info), rel[i].r_offset, ELF32_R_SYM(rel[i].r_info), rel[i].r_addend);
#endif
		switch (ELF32_R_TYPE(rel[i].r_info)) {
			case R_PPC_ADDR32:
			case R_PPC_ADDR24:
			case R_PPC_ADDR16:
			case R_PPC_ADDR16_LO:
			case R_PPC_ADDR16_HI:
			case R_PPC_ADDR16_HA:
			case R_PPC_REL24:
				sym = SYMBOL(image, ELF32_R_SYM(rel[i].r_info));

				vlErr = elf_resolve_symbol(image, sym, resolve_image, sym_prepend, &S);
				if (vlErr<0)
					return vlErr;
				break;
		}

		switch (ELF32_R_TYPE(rel[i].r_info)) {
			case R_PPC_NONE:
				continue;

			case R_PPC_ADDR32:
				final_val = S + A;
				break;

			case R_PPC_ADDR16:
				*(Elf32_Half*)P = S;
				continue;

			case R_PPC_ADDR16_LO:
				*(Elf32_Half*)P = S & 0xffff;
				continue;

			case R_PPC_ADDR16_HI:
				*(Elf32_Half*)P = (S >> 16) & 0xffff;
				continue;

			case R_PPC_ADDR16_HA:
				*(Elf32_Half*)P = ((S >> 16) + ((S & 0x8000) ? 1 : 0)) & 0xFFFF;
				continue;

			case R_PPC_REL24:
				final_val = (*(Elf32_Word *)P & 0xff000003) | ((S + A - P) & 0x3fffffc);
				break;

			case R_PPC_RELATIVE:
				final_val = B + A;
				break;

			default:
				dprintf("arch_elf_relocate_rela: unhandled relocation type %d\n", ELF32_R_TYPE(rel[i].r_info));
				return B_ERROR;
		}
#if CHATTY
		dprintf("going to put 0x%x at 0x%x\n", final_val, P);
#endif
		*(addr_t *)P = final_val;
	}

	return B_NO_ERROR;

}

