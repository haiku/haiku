/* 
** Distributed under the terms of the Haiku License.
*/
#ifndef _ASSERT_H_
#define _ASSERT_H_

#undef assert

#ifndef NDEBUG
	// defining NDEBUG disables assert() functionality

#ifdef __cplusplus
extern "C" {
#endif

extern void __assert_fail(const char *assertion, const char *file,
				unsigned int line, const char *function);

extern void __assert_perror_fail(int error, const char *file,
				unsigned int line, const char *function);

#ifdef __cplusplus
}
#endif

#define assert(assertion) \
	((assertion) ? (void)0 : __assert_fail(#assertion, __FILE__, __LINE__, __PRETTY_FUNCTION__))

#else	// NDEBUG
#	define assert(condition) ;
#endif

#endif	/* _ASSERT_H_ */
