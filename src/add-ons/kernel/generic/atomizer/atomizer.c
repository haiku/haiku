/*******************************************************************************
/
/	File:			atomizer.c
/
/	Description:	Kernel module implementing kernel-space atomizer API
/
/	Copyright 1999, Be Incorporated, All Rights Reserved.
/	This file may be used under the terms of the Be Sample Code License.
/
*******************************************************************************/

#include <Drivers.h>
#include <KernelExport.h>
#include <string.h>
#include <stdlib.h>

#include <atomizer.h>


#if DEBUG > 0
#define ddprintf(x) dprintf x
#else
#define ddprintf(x)
#endif

typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x)		x.sem = create_sem(0, "an atomizer benaphore");  x.ben = 0;
#define ACQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define ACQUIRE_BEN_ON_ERROR(x, y) if((atomic_add(&(x.ben), 1)) >= 1) if (acquire_sem(x.sem) != B_OK) y;
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem); x.sem = -1; x.ben = 1;

#define MIN_BLOCK_SIZE B_PAGE_SIZE

typedef struct block_list block_list;

struct block_list{
	block_list	*next;
	uint8		*free_space;
	uint32		free_bytes;
};

typedef struct {
	benaphore	lock;		/* serializes access to this atomizer */
	block_list	*blocks;	/* list of blocks holding atomized strings */
	uint32		count;		/* number of atoms in this list */
	uint32		slots;		/* total number of spaces in the allocated vector */
	uint8		**vector;	/* sorted vector of pointers to atomized strings */
} atomizer_data;

typedef struct atomizer_data_list atomizer_data_list;

struct atomizer_data_list {
	char				*name;	/* the atomizer's name */
	atomizer_data		*the_atomizer;	/* the atomizer data */
	atomizer_data_list	*next;	/* the next atomizer in the list */
};


/* Module static data */
const char atomizer_module_name[] = B_ATOMIZER_MODULE_NAME;
const char system_atomizer_name[] = B_SYSTEM_ATOMIZER_NAME;

static benaphore module_lock;
static atomizer_data_list *atomizer_list;

static atomizer_data *
make_atomizer(void) {
	atomizer_data *ad;

	/* get space for atomizer data */
	ad = (atomizer_data *)malloc(sizeof(atomizer_data));
	if (ad) {
		/* init locks */
		INIT_BEN(ad->lock);
		if (ad->lock.sem >= 0) {
			/* get first block for atomized strings */
			ad->blocks = (block_list *)malloc(MIN_BLOCK_SIZE);
			if (ad->blocks) {
				/* get vector for pointers to strings */
				ad->vector = (uint8 **)malloc(MIN_BLOCK_SIZE);
				if (ad->vector) {
					/* fill in the details */
					ad->blocks->next = 0;
					ad->blocks->free_space = ((uint8 *)ad->blocks)+sizeof(block_list);
					ad->blocks->free_bytes = MIN_BLOCK_SIZE - sizeof(block_list);
					ad->count = 0;
					ad->slots = MIN_BLOCK_SIZE / sizeof(uint8 *);
					goto exit0;
				}
				/* oops, vector allocation failed */
				free(ad->blocks);
			}
			/* oops, block list allocation failed */
			DELETE_BEN(ad->lock);
		}
		/* oops, sem creation failed */
		free(ad);
		ad = 0;
	}
exit0:
	ddprintf(("make_atomizer() returns %p\n", ad));
	return ad;
}

static void
free_atomizer(atomizer_data *ad) {
	block_list *bl, *blnext;

	/* Acquire the lock or fail trying.  Better to not free
	the data than corrupt the kernel heap */
	ACQUIRE_BEN_ON_ERROR(ad->lock, return);
	/* walk the block_list, freeing them up */
	bl = ad->blocks;
	while (bl) {
		blnext = bl->next;
		free(bl);
		bl = blnext;
	}
	/* release vector of pointers to atomized strings */
	free(ad->vector);
	/* delete the benaphore */
	DELETE_BEN(ad->lock);
	/* finaly, release the atomizer data */
	free(ad);
}

static const void *
find_or_make_atomizer(const char *string) {
	atomizer_data_list *adl = atomizer_list;
	atomizer_data_list *last = 0;
	void * at = (void *)0;

	/* null or zero-length string means system atomizer */
	if (!string || (strlen(string) == 0)) string = system_atomizer_name;

	/* lock the atomizer list or fail returning a bogus atomizer */
	ACQUIRE_BEN_ON_ERROR(module_lock, return at);
	ddprintf(("success locking module\n"));
	while (adl && strcmp(adl->name, string)) {
		last = adl;
		adl = adl->next;
	}
	if (adl) {
		at = (void *)adl->the_atomizer;
		ddprintf(("asked for %s, returning %p\n", string, at));
		goto exit0;
	}
	ddprintf(("didn't find atomizer %s, making it\n", string));

	adl = (atomizer_data_list *)malloc(sizeof(atomizer_data_list));
	if (adl) {
		adl->the_atomizer = make_atomizer();
		if (adl->the_atomizer) {
			adl->name = strdup(string);
			if (adl->name) {
				/* append to list */
				if (last) last->next = adl;
				else atomizer_list = adl;
				adl->next = 0;
				at = (void *)adl->the_atomizer;
				goto exit0;
			}
			free_atomizer(adl->the_atomizer);
		}
		free(adl);
	}
exit0:
	RELEASE_BEN(module_lock);
	ddprintf(("find_or_make_atomizer() returning %p\n", at));
	return at;
}

static status_t
init()
{
	ddprintf((B_ATOMIZER_MODULE_NAME": init()\n"));
	/* init the module-wide benaphore */
	INIT_BEN(module_lock);
	if (module_lock.sem >= 0) {
		/* create list of atomizers */
		atomizer_list = 0;
		if (find_or_make_atomizer(0))
			return B_OK;
		/* oops, couldn't create system atomizer */
		DELETE_BEN(module_lock);
		module_lock.sem = B_ERROR;
	}
	return module_lock.sem;
}

static status_t
uninit()
{
	atomizer_data_list *adl;
	/* aquire the module-wide lock.  This is pure paranoia.
	If it fails, all hell as broken loose, but we won't contribute
	by corrupting the heap. */
	ddprintf((B_ATOMIZER_MODULE_NAME": uninit()\n"));
	ACQUIRE_BEN_ON_ERROR(module_lock, return B_ERROR);
	if (atomizer_list->next) {
		ddprintf((B_ATOMIZER_MODULE_NAME": uninit called with non-system atomizers still active!\n"));
	}
	/* delete all of the atomizers.  Ideally, there should only
	be the system atomizer left */
	adl = atomizer_list;
	while (adl) {
		free_atomizer(adl->the_atomizer);
		free(adl->name);
		adl = adl->next;
	}
	/* free up the module-wide benaphore */
	DELETE_BEN(module_lock);
	return B_OK;
}

static status_t
std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		return init();
	case B_MODULE_UNINIT:
		return uninit();
	default:
		/* do nothing */
		;
	}
	return -1;
}

static status_t
delete_atomizer(const void * at) {
	atomizer_data_list *adl;
	atomizer_data_list *last = 0;
	status_t result = B_ERROR;

	ACQUIRE_BEN_ON_ERROR(module_lock, return B_ERROR);
	adl = atomizer_list;
	while (adl && (adl->the_atomizer != (atomizer_data *)at)) {
		last = adl;
		adl = adl->next;
	}

	if (adl) {
		/* don't let clients delete the system atomizer */
		if (strcmp(adl->name, system_atomizer_name) != 0) {
			free_atomizer(adl->the_atomizer);
			free(adl->name);
			/* the system atomizer is always first, so last will be non-zero */
			last->next = adl->next;
			free(adl);
			result = B_OK;
		}	
	}
	RELEASE_BEN(module_lock);
	return result;
}

static const void *
atomize(const void * at, const char *string, int create) {
	atomizer_data_list *adl;
	void *result = 0;
	uint32	len;

	/* NULL and zero length strings aren't interned */
	if (!string || !(len = strlen(string))) return 0;
	/* include the null terminator */
	len++;
	/* own the list or fail trying */
	ACQUIRE_BEN_ON_ERROR(module_lock, return 0);

	/* use the system atomizer if none specified */
	if (at == ((void *)(-1))) at = atomizer_list->the_atomizer;

	/* walk the list of known atomizers, looking for a match */
	adl = atomizer_list;
	while (adl && (adl->the_atomizer != (atomizer_data *)at)) {
		adl = adl->next;
	}
	/* lock the atomizer *before* we give up control over the list */
	if (adl) ACQUIRE_BEN(adl->the_atomizer->lock);
	/* allow list manipulations */
	RELEASE_BEN(module_lock);
	/* did we find the one we wanted? */
	if (adl) {
		atomizer_data *ad = adl->the_atomizer;
		uint8 **vector = ad->vector;
		uint32 count = ad->count;
		uint32 low = 0;
		uint32 high = count;
		uint32 index = 0;
		int test = -1;
		/* do a binary search on the vector for a match */
		while (low < high) {
			index = (low + high) / 2;
			test = strcmp(string, (const char *)vector[index]);
			if (test < 0)
				high = index;
			else if (test > 0)
				low = ++index;	// if we exit the while loop here, use index + 1					
			else
				break;
		}
		/* if match found, note result and return it */
		if (test == 0) {
			result = vector[index];
		} else {
			/* if we're not "just checking" */
			if (create) {
				/* no match, so add to list */
				/* find first block with enough room */
				block_list *bl = ad->blocks;
				while (bl->next) {
					if (bl->free_bytes >= len) break;
					bl = bl->next;
				}
				if (bl->free_bytes < len) {
					/* allocate a block big enough to handle the string */
					size_t block_size = ((sizeof(block_list)+len+(MIN_BLOCK_SIZE-1)) & ~(MIN_BLOCK_SIZE-1));
					bl->next = (block_list *)malloc(block_size);
					if (!bl->next) {
						RELEASE_BEN(ad->lock);
						return 0;
					}
					bl = bl->next;
					bl->free_space = (uint8 *)(bl+1);
					bl->free_bytes = block_size - sizeof(block_list);
				}
				/* actually intern an atom */
				result = bl->free_space;
				memcpy(result, string, len);
				bl->free_space += len;
				bl->free_bytes -= len;
				
				/* insert the pointer in the vector */
				/* if the vector isn't big enough, realloc it */
				if (ad->count == ad->slots) {
					size_t newslots = ad->slots + (MIN_BLOCK_SIZE / sizeof(uint8 *));
					uint8 **newvec = realloc(vector, newslots * sizeof(uint8 *));
					if (!newvec) {
						/* bail, leaving the atom un-interned */
						RELEASE_BEN(ad->lock);
						return 0;
					}
					ad->slots = newslots;
					vector = ad->vector = newvec;
				}
				/* move all of the entries starting at index down one slot */
				memmove(vector + index + 1, vector + index, (count - index) * sizeof(uint8 *));
				/* insert the new pointer in the vector */
				vector[index] = result;
				/* note new count */
				ad->count++;
			}
		}
		RELEASE_BEN(ad->lock);
	}
	return result;
}

static const char *
string_for_token(const void *atomizer, const void *atom) {
	/* someday, check for validity */
	return atom;
}

static status_t
get_next_atomizer_info(void **cookie, atomizer_info *info) {
	atomizer_data_list *adl;
	atomizer_data_list *atomizer = (atomizer_data_list *)*cookie;
	status_t result = B_ERROR;

	ddprintf(("get_next_atomizer_info(cookie: %p, info: %p)\n", *cookie, info));
	/* a NULL info pointer? */
	if (!info) return result;

	ACQUIRE_BEN_ON_ERROR(module_lock, return result);
	/* null? start over */
	if (!atomizer) atomizer = atomizer_list;
	adl = atomizer_list;
	while (adl && (adl != atomizer))
		adl = adl->next;
	if (adl) {
		*cookie = atomizer->next;
		/* use a bogus cookie if no more left */
		if (!*cookie) *cookie = (void *)-1;
		info->atomizer = atomizer->the_atomizer;
		strncpy(info->name, adl->name, B_OS_NAME_LENGTH);
		info->name[B_OS_NAME_LENGTH - 1] = '\0';
		info->atom_count = atomizer->the_atomizer->count;
		result = B_OK;
	}
	RELEASE_BEN(module_lock);
	return result;
}

static const void *
get_next_atom(const void *_atomizer, uint32 *cookie) {
	atomizer_data_list *adl;
	atomizer_data *atomizer = (atomizer_data *)_atomizer;
	void *result = 0;

	/* validate the atomizer */
	ACQUIRE_BEN_ON_ERROR(module_lock, return 0);
	adl = atomizer_list;
	while (adl && (adl->the_atomizer != atomizer))
		adl = adl->next;
	/* lock the atomizer *before* we give up control over the list */
	if (adl) ACQUIRE_BEN(atomizer->lock);
	/* allow list manipulations */
	RELEASE_BEN(module_lock);
	if (adl) {
		if (*cookie < atomizer->count) {
			result = atomizer->vector[*cookie];
			(*cookie)++;
		}
		RELEASE_BEN(atomizer->lock);
	}
	return result;
}

static atomizer_module_info atomizer = {
	{
		atomizer_module_name,
		0,
		std_ops
	},
	find_or_make_atomizer,
	delete_atomizer,
	atomize,
	string_for_token,
	get_next_atomizer_info,
	get_next_atom
};

_EXPORT atomizer_module_info *modules[] = {
        &atomizer,
        NULL
};

