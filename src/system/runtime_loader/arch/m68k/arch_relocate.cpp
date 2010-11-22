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


//#define TRACE_RLD
#ifdef TRACE_RLD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static inline void
write_32(addr_t *P, Elf32_Word value)
{
	*(Elf32_Word*)P = value;
}


static inline void
write_16(addr_t *P, Elf32_Word value)
{
	// bits 16:29
	*(Elf32_Half*)P = (Elf32_Half)value;
}


static inline bool
write_16_check(addr_t *P, Elf32_Word value)
{
	// bits 15:0
	if ((value & 0xffff0000) && (~value & 0xffff8000))
		return false;
	*(Elf32_Half*)P = (Elf32_Half)value;
	return true;
}


static inline bool
write_8(addr_t *P, Elf32_Word value)
{
	// bits 7:0
	*(uint8 *)P = (uint8)value;
	return true;
}


static inline bool
write_8_check(addr_t *P, Elf32_Word value)
{
	// bits 7:0
	if ((value & 0xffffff00) && (~value & 0xffffff80))
		return false;
	*(uint8 *)P = (uint8)value;
	return true;
}


static int
relocate_rela(image_t *rootImage, image_t *image, struct Elf32_Rela *rel,
	int rel_len, SymbolLookupCache* cache)
{
	int i;
	addr_t S;
	addr_t final_val;

# define P	((addr_t *)(image->regions[0].delta + rel[i].r_offset))
//# define A	(*(P))
#define A	((addr_t)rel[i].r_addend)
# define B	(image->regions[0].delta)

	for (i = 0; i * (int)sizeof(struct Elf32_Rel) < rel_len; i++) {
		unsigned type = ELF32_R_TYPE(rel[i].r_info);

		switch (type) {
			case R_68K_32:
			case R_68K_16:
			case R_68K_8:
			case R_68K_PC32:
			case R_68K_PC16:
			case R_68K_PC8:
			case R_68K_GLOB_DAT:
			case R_68K_JMP_SLOT:
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
			case R_68K_NONE:
				continue;
			case R_68K_32:
				write_32(P, S + A);
				break;
			case R_68K_16:
				if (write_16_check(P, S + A))
					break;
				TRACE(("R_68K_16 overflow\n"));
				return B_BAD_DATA;

			case R_68K_8:
				if (write_8_check(P, S + A))
					break;
				TRACE(("R_68K_8 overflow\n"));
				return B_BAD_DATA;

			case R_68K_PC32:
				write_32(P, (S + A - (addr_t)P));
				break;

#if 0
			case R_68K_PC16:
				if (write_16_check(P, (S + A - P)))
					break;
				TRACE(("R_68K_PC16 overflow\n"));
				return B_BAD_DATA;

			case R_68K_PC8:
				if (write_8(P, (S + A - P)))
					break;
				TRACE(("R_68K_PC8 overflow\n"));
				return B_BAD_DATA;

			case R_68K_GOT32:
				REQUIRE_GOT;
				write_32(P, (G + A - P));
				break;

			case R_68K_GOT16:
				REQUIRE_GOT;
				if (write_16_check(P, (G + A - P)))
					break;
				TRACE(("R_68K_GOT16 overflow\n"));
				return B_BAD_DATA;

			case R_68K_GOT8:
				REQUIRE_GOT;
				if (write_8_check(P, (G + A - P)))
					break;
				TRACE(("R_68K_GOT8 overflow\n"));
				return B_BAD_DATA;

			case R_68K_GOT32O:
				REQUIRE_GOT;
				write_32(P, (G + A));
				break;

			case R_68K_GOT16O:
				REQUIRE_GOT;
				if (write_16_check(P, (G + A)))
					break;
				TRACE(("R_68K_GOT16 overflow\n"));
				return B_BAD_DATA;

			case R_68K_GOT8O:
				REQUIRE_GOT;
				if (write_8_check(P, (G + A)))
					break;
				TRACE(("R_68K_GOT8 overflow\n"));
				return B_BAD_DATA;

			case R_68K_PLT32:
				REQUIRE_PLT;
				write_32(P, (L + A - P));
				break;

			case R_68K_PLT16:
				REQUIRE_PLT;
				if (write_16_check(P, (L + A - P)))
					break;
				TRACE(("R_68K_PLT16 overflow\n"));
				return B_BAD_DATA;

			case R_68K_PLT8:
				REQUIRE_PLT;
				if (write_8_check(P, (L + A - P)))
					break;
				TRACE(("R_68K_PLT8 overflow\n"));
				return B_BAD_DATA;

			case R_68K_PLT32O:
				REQUIRE_PLT;
				write_32(P, (L + A));
				break;

			case R_68K_PLT16O:
				REQUIRE_PLT;
				if (write_16_check(P, (L + A)))
					break;
				TRACE(("R_68K_PLT16O overflow\n"));
				return B_BAD_DATA;

			case R_68K_PLT8O:
				REQUIRE_PLT;
				if (write_8_check(P, (L + A)))
					break;
				TRACE(("R_68K_PLT8O overflow\n"));
				return B_BAD_DATA;
			case R_386_GOT32:
				final_val = G + A;
				break;
			case R_386_PLT32:
				final_val = L + A - (addr_t)P;
				break;
#endif
			case R_68K_COPY:
				/* what ? */
				continue;
			case R_68K_GLOB_DAT:
				write_32(P, S/* + A*/);
				break;
			case R_68K_JMP_SLOT:
				//XXX ? write_32(P, (G + A));
				write_32(P, S);
				break;
#if 0
			case R_386_JMP_SLOT:
				write_32(P, S);
				break;
#endif
			case R_68K_RELATIVE:
				write_32(P, B + A);
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
arch_relocate_image(image_t *rootImage, image_t *image,
	SymbolLookupCache* cache)
{
	status_t status;

	// deal with the rels first
	if (image->rel) {
		TRACE(("RELA relocations not supported\n"));
		return EOPNOTSUPP;
	}

	if (image->pltrel) {
		TRACE(("RELA relocations not supported\n"));
		return EOPNOTSUPP;
#if 0
		status = relocate_rel(rootImage, image, image->pltrel,
			image->pltrel_len);
		if (status < B_OK)
			return status;
#endif
	}

	if (image->rela) {
		status = relocate_rela(rootImage, image, image->rela, image->rela_len,
			cache);
		//int i;
		if (status < B_OK)
			return status;
		//TRACE(("RELA relocations not supported\n"));
		//return EOPNOTSUPP;

		//for (i = 1; i * (int)sizeof(struct Elf32_Rela) < image->rela_len; i++) {
		//	printf("rela: type %d\n", ELF32_R_TYPE(image->rela[i].r_info));
		//}
	}

#if 0
	if (image->pltrela) {
		status = relocate_rela(rootImage, image, image->pltrela,
			image->pltrela_len);
		if (status < B_OK)
			return status;
	}
#endif

	return B_OK;
}
