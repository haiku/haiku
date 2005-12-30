/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <boot/arch.h>

#include <KernelExport.h>

#include <elf_priv.h>
#include <arch/elf.h>


//#define TRACE_ARCH_ELF
#ifdef TRACE_ARCH_ELF
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#ifdef TRACE_ARCH_ELF
static const char *kRelocations[] = {
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
};
#endif


status_t
boot_arch_elf_relocate_rel(struct preloaded_image *image,
	struct Elf32_Rel *rel, int rel_len)
{
	struct Elf32_Sym *sym;
	addr_t S;
	addr_t A;
	addr_t P;
	addr_t final_val;
	int i;

	S = A = P = 0;

	for (i = 0; i * (int)sizeof(struct Elf32_Rel) < rel_len; i++) {
		TRACE(("looking at rel type %s, offset 0x%lx\n", kRelocations[ELF32_R_TYPE(rel[i].r_info)], rel[i].r_offset));

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

				vlErr = boot_elf_resolve_symbol(image, sym, &S);
				if (vlErr < 0)
					return vlErr;
				TRACE(("S %p (%s)\n", (void *)S, SYMNAME(image, sym)));
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
		TRACE(("-> offset %p = %p\n", (void *)(image->text_region.delta + rel[i].r_offset), (void *)final_val));
	}

	return B_NO_ERROR;
}


status_t
boot_arch_elf_relocate_rela(struct preloaded_image *image,
	struct Elf32_Rela *rel, int rel_len)
{
	dprintf("arch_elf_relocate_rela: not supported on x86\n");
	return B_ERROR;
}

