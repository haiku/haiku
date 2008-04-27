/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_PARANOIA_H
#define _KERNEL_DEBUG_PARANOIA_H

#include <sys/cdefs.h>

#include <SupportDefs.h>

#include "paranoia_config.h"


// How to use: Include the header only from source files. Define
// ENABLE_PARANOIA_CHECK_COMPONENT (to 1 to enable) before. Use the macros
// defined below and the ParanoiaChecker class.


enum paranoia_set_check_mode {
	PARANOIA_DONT_FAIL,
	PARANOIA_FAIL_IF_EXISTS,
	PARANOIA_FAIL_IF_MISSING
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


#if ENABLE_PARANOIA_CHECK_COMPONENT
#	define PARANOIA_ONLY(x)	x
#else
#	define PARANOIA_ONLY(x)
#endif

#define	CREATE_PARANOIA_CHECK_SET(object, description)	\
			PARANOIA_ONLY(create_paranoia_check_set((object), (description)))
#define	DELETE_PARANOIA_CHECK_SET(object)	\
			PARANOIA_ONLY(delete_paranoia_check_set((object)))
#define	RUN_PARANOIA_CHECKS(object)	\
			PARANOIA_ONLY(run_paranoia_checks((object)))
#define	ADD_PARANOIA_CHECK(object, address, size)	\
			PARANOIA_ONLY(set_paranoia_check((object), (address), (size), \
				PARANOIA_FAIL_IF_EXISTS))
#define	UPDATE_PARANOIA_CHECK(object, address, size)	\
			PARANOIA_ONLY(set_paranoia_check((object), (address), (size), \
				PARANOIA_FAIL_IF_MISSING))
#define	SET_PARANOIA_CHECK(object, address, size)	\
			PARANOIA_ONLY(set_paranoia_check((object), (address), (size), \
				PARANOIA_DONT_FAIL))
#define	REMOVE_PARANOIA_CHECK(object, address, size)	\
			PARANOIA_ONLY(remove_paranoia_check((object), (address), (size)))


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
