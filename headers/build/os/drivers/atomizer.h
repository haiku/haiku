/*******************************************************************************
/
/	File:			atomizer.h
/
/	Description:	Kernel atomizer module API
/
/	Copyright 1999, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _ATOMIZER_MODULE_H_
#define _ATOMIZER_MODULE_H_

#include <module.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
	An atomizer is a software device that returns a unique token for a
	null-terminated UTF8 string.

	Each atomizer comprises a separate token space.  The same string interned
	in two different atomizers will generate two distinct tokens.

	Atomizers and the tokens they generate are only guaranteed valid between
	matched calls to get_module/put_module.
	
	void * find_or_make_atomizer(const char *string)
		Returns a token that identifies the named atomizer, creating a new
		atomizer if the named atomizer does not exist.  Pass null, a zero
		length string, or the value B_SYSTEM_ATOMIZER_NAME for string will
		return a pointer to the system atomizer.  Returns (void *)(0)
		if the atomizer could not be created (for whatever reason).  A return
		value of (void *)(-1) refers to the system atomizer.

	status_t delete_atomizer(void *atomizer)
		Delete the atomizer specified.  Returns B_OK if successfull, B_ERROR
		otherwise.  An error return usually means that a race condition was
		detected while destroying the atomizer.

	void * atomize(void *atomizer, const char *string, int create)
		Return the unique token for the specified string, creating a new token
		if the string was not previously atomized and create is non-zero.  If
		atomizer is (void *)(-1), use the system atomizer (saving the step of
		looking it up with find_or_make_atomizer().  Returns (const char *)(0)
		if there were any errors detected: insufficient memory or a race
		condition with someone deleting the atomizer.

	const char * string_for_token(void *atomizer, void *atom)
		Return a pointer to the string described by atom in the provided atomizer.
		Returns (const char *)(0) if either the atomizer or the atom were invalid.

	status_t get_next_atomizer_info(void **cookie, atomizer_info *info)
		Returns info about the next atomizer in the list of atomizers by modifying
		the contents of info.  The pointer specified by *cookie should be set to
		(void *)(0) to retrieve the first atomizer, and should not be modified
		thereafter.  Returns B_ERROR when there are no more atomizers.
		Adding or deleting atomizers between calls to get_next_atomizer() results
		in a safe but undefined behavior.

	void * get_next_atom(void *atomizer, uint32 *cookie)
		Returns the next atom interned in specified atomizer,  *cookie
		should be set to (uint32)(0) to get the first atom.  Returns
		(void *)(0) when there are no more atoms.  Adding atoms between
		calls to get_next_atom() may cause atoms to be skipped.

	Atomizers are SMP-safe.  Check return codes for errors!

*/

#define B_ATOMIZER_MODULE_NAME "generic/atomizer/v1"
#define B_SYSTEM_ATOMIZER_NAME "BeOS System Atomizer"

typedef struct {
	void	*atomizer;	/* An opaque token representing the atomizer. */
	char	name[B_OS_NAME_LENGTH];	/* The first B_OS_NAME_LENGTH bytes of the atomizer name, null terminated. */
	uint32	atom_count;	/* The number of atoms currently interned in this atomizer. */
} atomizer_info;

typedef struct {
	module_info		minfo;
	const void *	(*find_or_make_atomizer)(const char *string);
	status_t		(*delete_atomizer)(const void *atomizer);
	const void *	(*atomize)(const void *atomizer, const char *string, int create);
	const char *	(*string_for_token)(const void * atomizer, const void *atom);
	status_t		(*get_next_atomizer_info)(void **cookie, atomizer_info *info);
	const void *	(*get_next_atom)(const void *atomizer, uint32 *cookie);
} atomizer_module_info;

#ifdef __cplusplus
}
#endif

#endif

