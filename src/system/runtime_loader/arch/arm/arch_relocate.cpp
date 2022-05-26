/*
 * Copyright 2012-2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "runtime_loader_private.h"

#include <runtime_loader.h>

//#define TRACE_RLD
#ifdef TRACE_RLD
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


static int
relocate_rel(image_t *rootImage, image_t *image, Elf32_Rel *rel, int rel_len,
	SymbolLookupCache* cache)
{
# define P	((addr_t *)(image->regions[0].delta + rel[i].r_offset))
# define A	(*(P))
# define B	(image->regions[0].delta)

	for (int i = 0; i * (int)sizeof(Elf32_Rel) < rel_len; i++) {
		unsigned type = ELF32_R_TYPE(rel[i].r_info);
		unsigned symbolIndex = ELF32_R_SYM(rel[i].r_info);
		addr_t final_val;
		addr_t S;

		image_t* symbolImage = NULL;
		if (symbolIndex != 0) {
			Elf32_Sym* sym = SYMBOL(image, symbolIndex);
			status_t status = resolve_symbol(rootImage, image, sym, cache, &S,
					&symbolImage);
			if (status < B_OK) {
				TRACE("resolve symbol \"%s\" returned: %ld\n",
					SYMNAME(image, sym), status);
				return status;
			}
		}

		switch (type) {
			case R_ARM_NONE:
				continue;
			case R_ARM_RELATIVE:
				final_val = B + A;
				break;
			case R_ARM_JMP_SLOT:
			case R_ARM_GLOB_DAT:
				final_val = S;
				break;
			case R_ARM_ABS32:
				final_val = S + A;
				break;
			default:
				TRACE("unhandled relocation type %d\n", ELF32_R_TYPE(rel[i].r_info));
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
arch_relocate_image(image_t *rootImage, image_t *image,
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
		TRACE("RELA relocations not supported\n");
		return EOPNOTSUPP;
	}

	return B_OK;
}
