/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

static
int
relocate_rel(image_t *image, struct Elf32_Rel *rel, int rel_len )
{
	int i;
	struct Elf32_Sym *sym;
	int vlErr;
	addr S;
	addr final_val;

# define P         ((addr *)(image->regions[0].delta + rel[i].r_offset))
# define A         (*(P))
# define B         (image->regions[0].delta)

	for(i = 0; i * (int)sizeof(struct Elf32_Rel) < rel_len; i++) {

		unsigned type= ELF32_R_TYPE(rel[i].r_info);

		switch(ELF32_R_TYPE(rel[i].r_info)) {
			case R_386_32:
			case R_386_PC32:
			case R_386_GLOB_DAT:
			case R_386_JMP_SLOT:
			case R_386_GOTOFF:
				sym = SYMBOL(image, ELF32_R_SYM(rel[i].r_info));

				vlErr = resolve_symbol(image, sym, &S);
				if(vlErr<0) {
					return vlErr;
				}
		}
		switch(type) {
			case R_386_NONE:
				continue;
			case R_386_32:
				final_val= S+A;
				break;
			case R_386_PC32:
				final_val=S+A-(addr)P;
				break;
#if 0
			case R_386_GOT32:
				final_val= G+A;
				break;
			case R_386_PLT32:
				final_val= L+A-(addr)P;
				break;
#endif
			case R_386_COPY:
				/* what ? */
				continue;
			case R_386_GLOB_DAT:
				final_val= S;
				break;
			case R_386_JMP_SLOT:
				final_val= S;
				break;
			case R_386_RELATIVE:
				final_val= B+A;
				break;
#if 0
			case R_386_GOTOFF:
				final_val= S+A-GOT;
				break;
			case R_386_GOTPC:
				final_val= GOT+A-P;
				break;
#endif
			default:
				printf("unhandled relocation type %d\n", ELF32_R_TYPE(rel[i].r_info));
				return ERR_NOT_ALLOWED;
		}

		*P= final_val;
	}

# undef P
# undef A
# undef B


	return B_NO_ERROR;
}

/*
 * rldelf.c requires this function to be implemented on a per-cpu basis
 */
static
bool
relocate_image(image_t *image)
{
	int res = B_NO_ERROR;
	int i;

	if(image->flags & RFLAG_RELOCATED) {
		return true;
	}
	image->flags|= RFLAG_RELOCATED;

	// deal with the rels first
	if(image->rel) {
		res= relocate_rel( image, image->rel, image->rel_len );

		if(res) {
			return false;
		}
	}

	if(image->pltrel) {
		res= relocate_rel(image, image->pltrel, image->pltrel_len);

		if(res) {
			return false;
		}
	}

	if(image->rela) {
		printf("RELA relocations not supported\n");
		return ERR_NOT_ALLOWED;
		for(i = 1; i * (int)sizeof(struct Elf32_Rela) < image->rela_len; i++) {
			printf("rela: type %d\n", ELF32_R_TYPE(image->rela[i].r_info));
		}
	}

	return true;
}
