/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "runtime_loader_private.h"

#include <runtime_loader.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


//#define TRACE_RLD
#ifdef TRACE_RLD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static int
relocate_rel(image_t *rootImage, image_t *image, struct Elf32_Rel *rel,
	int rel_len, SymbolLookupCache* cache)
{
	// ToDo: implement me!

	return B_NO_ERROR;
}


status_t
arch_relocate_image(image_t *rootImage, image_t *image,
	SymbolLookupCache* cache)
{
	status_t status = B_NO_ERROR;

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
		printf("RELA relocations not supported\n");
		return EOPNOTSUPP;

		//for (i = 1; i * (int)sizeof(struct Elf32_Rela) < image->rela_len; i++) {
		//	printf("rela: type %d\n", ELF32_R_TYPE(image->rela[i].r_info));
		//}
	}

	return B_OK;
}
