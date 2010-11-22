/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"

#include <runtime_loader.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


static int
relocate_rel(image_t *rootImage, image_t *image, struct Elf32_Rel *rel,
	int rel_len, SymbolLookupCache* cache)
{
	int i;
	addr_t S;
	addr_t final_val;

# define P	((addr_t *)(image->regions[0].delta + rel[i].r_offset))
# define A	(*(P))
# define B	(image->regions[0].delta)

	for (i = 0; i * (int)sizeof(struct Elf32_Rel) < rel_len; i++) {
		unsigned type = ELF32_R_TYPE(rel[i].r_info);

		switch (type) {
			case R_386_32:
			case R_386_PC32:
			case R_386_GLOB_DAT:
			case R_386_JMP_SLOT:
			case R_386_GOTOFF:
			{
				struct Elf32_Sym *sym;
				status_t status;
				sym = SYMBOL(image, ELF32_R_SYM(rel[i].r_info));

				status = resolve_symbol(rootImage, image, sym, cache, &S);
				if (status < B_OK) {
					TRACE(("resolve symbol \"%s\" returned: %ld\n",
						SYMNAME(image, sym), status));
					printf("resolve symbol \"%s\" returned: %ld\n",
						SYMNAME(image, sym), status);
					return status;
				}
			}
		}
		switch (type) {
			case R_386_NONE:
				continue;
			case R_386_32:
				final_val = S + A;
				break;
			case R_386_PC32:
				final_val = S + A - (addr_t)P;
				break;
#if 0
			case R_386_GOT32:
				final_val = G + A;
				break;
			case R_386_PLT32:
				final_val = L + A - (addr_t)P;
				break;
#endif
			case R_386_COPY:
				/* what ? */
				continue;
			case R_386_GLOB_DAT:
				final_val = S;
				break;
			case R_386_JMP_SLOT:
				final_val = S;
				break;
			case R_386_RELATIVE:
				final_val = B + A;
				break;
#if 0
			case R_386_GOTOFF:
				final_val = S + A - GOT;
				break;
			case R_386_GOTPC:
				final_val = GOT + A - P;
				break;
#endif
			default:
				TRACE(("unhandled relocation type %d\n", ELF32_R_TYPE(rel[i].r_info)));
				return B_NOT_ALLOWED;
		}

		*P = final_val;
	}

# undef P
# undef A
# undef B

	return B_NO_ERROR;
}


status_t
arch_relocate_image(image_t* rootImage, image_t* image,
	SymbolLookupCache* cache)
{
	status_t status;

	// deal with the rels first
	if (image->rel) {
		status = relocate_rel(rootImage, image, image->rel, image->rel_len,
			cache);
		if (status < B_OK)
			return status;
	}

	if (image->pltrel) {
		status = relocate_rel(rootImage, image, image->pltrel,
			image->pltrel_len, cache);
		if (status < B_OK)
			return status;
	}

	if (image->rela) {
		//int i;
		TRACE(("RELA relocations not supported\n"));
		return EOPNOTSUPP;

		//for (i = 1; i * (int)sizeof(struct Elf32_Rela) < image->rela_len; i++) {
		//	printf("rela: type %d\n", ELF32_R_TYPE(image->rela[i].r_info));
		//}
	}

	return B_OK;
}
