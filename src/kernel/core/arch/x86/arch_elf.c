/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <KernelExport.h>

#include <elf_priv.h>
#include <arch/elf.h>

#define ELF_TRACE 0
#if ELF_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


int
arch_elf_relocate_rel(struct elf_image_info *image, const char *sym_prepend,
	struct elf_image_info *resolve_image, struct Elf32_Rel *rel, int rel_len)
{
	struct Elf32_Sym *sym;
	addr_t S;
	addr_t A;
	addr_t P;
	addr_t final_val;
	int i;

	S = A = P = 0;

	for (i = 0; i * (int)sizeof(struct Elf32_Rel) < rel_len; i++) {
		TRACE(("looking at rel type %d, offset 0x%x\n", ELF32_R_TYPE(rel[i].r_info), rel[i].r_offset));

		// calc S
		switch (ELF32_R_TYPE(rel[i].r_info)) {
			case R_386_32:
			case R_386_PC32:
			case R_386_GLOB_DAT:
			case R_386_JMP_SLOT:
			case R_386_GOTOFF:
			{
				int vlErr;

				sym = SYMBOL(image, ELF32_R_SYM(rel[i].r_info));

				vlErr = elf_resolve_symbol(image, sym, resolve_image, sym_prepend, &S);
				if (vlErr < 0)
					return vlErr;
				TRACE(("S %p\n", (void *)S));
			}
		}
		// calc A
		switch (ELF32_R_TYPE(rel[i].r_info)) {
			case R_386_32:
			case R_386_PC32:
			case R_386_GOT32:
			case R_386_PLT32:
			case R_386_RELATIVE:
			case R_386_GOTOFF:
			case R_386_GOTPC:
				A = *(addr_t *)(image->text_region.delta + rel[i].r_offset);
				TRACE(("A %p\n", (void *)A));
				break;
		}
		// calc P
		switch (ELF32_R_TYPE(rel[i].r_info)) {
			case R_386_PC32:
			case R_386_GOT32:
			case R_386_PLT32:
			case R_386_GOTPC:
				P = image->text_region.delta + rel[i].r_offset;
				TRACE(("P %p\n", (void *)P));
				break;
		}

		switch (ELF32_R_TYPE(rel[i].r_info)) {
			case R_386_NONE:
				continue;
			case R_386_32:
				final_val = S + A;
				break;
			case R_386_PC32:
				final_val = S + A - P;
				break;
			case R_386_RELATIVE:
				// B + A;
				final_val = image->text_region.delta + A;
				break;
			case R_386_JMP_SLOT:
			case R_386_GLOB_DAT:
				final_val = S;
				break;

			default:
				dprintf("arch_elf_relocate_rel: unhandled relocation type %d\n", ELF32_R_TYPE(rel[i].r_info));
				return EPERM;
		}
		*(addr_t *)(image->text_region.delta + rel[i].r_offset) = final_val;
	}

	return B_NO_ERROR;
}


int
arch_elf_relocate_rela(struct elf_image_info *image, const char *sym_prepend,
	struct elf_image_info *resolve_image, struct Elf32_Rela *rel, int rel_len)
{
	dprintf("arch_elf_relocate_rela: not supported on x86\n");
	return B_ERROR;
}

