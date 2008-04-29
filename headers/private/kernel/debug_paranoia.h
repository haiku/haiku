/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_PARANOIA_H
#define _KERNEL_DEBUG_PARANOIA_H

#include <sys/cdefs.h>

#include <SupportDefs.h>

#include "paranoia_config.h"


// How to use: Include the header only from source files. Set
// COMPONENT_PARANOIA_LEVEL before. Use the macros defined below and the
// ParanoiaChecker class.

// paranoia levels
#define PARANOIA_NAIVE		0	/* don't do any checks */
#define PARANOIA_SUSPICIOUS	1	/* do some checks */
#define PARANOIA_OBSESSIVE	2	/* do a lot of checks */
#define PARANOIA_INSANE		3	/* do all checks, also very expensive ones */

// mode for set_paranoia_check()
enum paranoia_set_check_mode {
	PARANOIA_DONT_FAIL,			// succeed, if check for address exists or not
	PARANOIA_FAIL_IF_EXISTS,	// fail, if check for address already exists
	PARANOIA_FAIL_IF_MISSING	// fail, if check for address doesn't exist yet
};


__BEGIN_DECLS

#if ENABLE_PARANOIA_CHECKS

status_t	create_paranoia_check_set(const void* object,
				const char* description);
status_t	delete_paranoia_check_set(const void* object);
status_t	run_paranoia_checks(const void* object);

status_t	set_paranoia_check(const void* object, const void* address,
				size_t size, paranoia_set_check_mode mode);
status_t	remove_paranoia_check(const void* object, const void* address,
				size_t size);

#endif	// ENABLE_PARANOIA_CHECKS

void		debug_paranoia_init();

__END_DECLS


#if ENABLE_PARANOIA_CHECKS && COMPONENT_PARANOIA_LEVEL
#	define PARANOIA_ONLY(x)	x
#	define PARANOIA_ONLY_LEVEL(level, x)				\
		if ((level) <= (COMPONENT_PARANOIA_LEVEL)) {	\
			x;											\
		}
#else
#	define PARANOIA_ONLY(x)
#	define PARANOIA_ONLY_LEVEL(level, x)
#endif

#define	CREATE_PARANOIA_CHECK_SET(object, description)	\
			PARANOIA_ONLY(create_paranoia_check_set((object), (description)))
#define	DELETE_PARANOIA_CHECK_SET(object)	\
			PARANOIA_ONLY(delete_paranoia_check_set((object)))
#define	RUN_PARANOIA_CHECKS(object)	\
			PARANOIA_ONLY(run_paranoia_checks((object)))

#define	ADD_PARANOIA_CHECK(level, object, address, size)		\
			PARANOIA_ONLY_LEVEL((level),						\
				set_paranoia_check((object), (address), (size), \
					PARANOIA_FAIL_IF_EXISTS))
#define	UPDATE_PARANOIA_CHECK(level, object, address, size)		\
			PARANOIA_ONLY_LEVEL((level),						\
				set_paranoia_check((object), (address), (size), \
					PARANOIA_FAIL_IF_MISSING))
#define	SET_PARANOIA_CHECK(level, object, address, size)		\
			PARANOIA_ONLY_LEVEL((level),						\
				set_paranoia_check((object), (address), (size), \
					PARANOIA_DONT_FAIL))
#define	REMOVE_PARANOIA_CHECK(level, object, address, size)		\
			PARANOIA_ONLY_LEVEL((level),						\
				remove_paranoia_check((object), (address), (size)))


#ifdef __cplusplus

class ParanoiaChecker {
public:
	inline ParanoiaChecker(const void* object)
	{
		PARANOIA_ONLY(fObject = object);
		RUN_PARANOIA_CHECKS(fObject);
	}

	inline ~ParanoiaChecker()
	{
		PARANOIA_ONLY(
			if (fObject != NULL)
				RUN_PARANOIA_CHECKS(fObject);
		)
	}

	inline void Disable()
	{
		PARANOIA_ONLY(fObject = NULL);
	}

private:
	PARANOIA_ONLY(
		const void*	fObject;
	)
};

#endif	// __cplusplus


#endif	// _KERNEL_DEBUG_PARANOIA_H
