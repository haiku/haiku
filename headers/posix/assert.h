/*
 * Copyright 2004-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ASSERT_H_
#define _ASSERT_H_


#undef assert

#ifndef NDEBUG
	/* defining NDEBUG disables assert() functionality */

#ifdef __cplusplus
extern "C" {
#endif

extern void __assert_fail(const char *assertion, const char *file,
				unsigned int line, const char *function)
	__attribute__ ((noreturn));

extern void __assert_perror_fail(int error, const char *file,
				unsigned int line, const char *function)
	__attribute__ ((noreturn));

#ifdef __cplusplus
}
#endif

#define assert(assertion) \
	((assertion) ? (void)0 : __assert_fail(#assertion, __FILE__, __LINE__, __PRETTY_FUNCTION__))

#else	/* NDEBUG */
#	define assert(condition) ((void)0)
#endif

#endif	/* _ASSERT_H_ */
