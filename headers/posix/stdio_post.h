/* 
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _STDIO_POST_H_
#define _STDIO_POST_H_

// "Private"/inline functions of our BeOS compatible stdio implementation

// ToDo: this is a work in progress to make our stdio
//		BeOS' GNU/libio (almost) binary compatible
//		We are not yet compatible!
//		Currently only function names are compatible

#ifndef _STDIO_H_
#	error "This file must be included from stdio.h!"
#endif

extern char _single_threaded;
	// this boolean value is true (1) if there is only the main thread
	// running - as soon as you spawn the first thread, it's set to
	// false (0)

#ifdef __cplusplus
#	define __INLINE inline
#else
#	define __INLINE extern __inline
#endif


__INLINE int
feof_unlocked(FILE *stream)
{
	return _IO_feof_unlocked(stream);
}


__INLINE int
ferror_unlocked(FILE *stream)
{
	return _IO_ferror_unlocked(stream);
}


#define getc(stream) \
	(_single_threaded ? _IO_getc_unlocked(stream) : _IO_getc(stream))
#define putc(c, stream) \
	(_single_threaded ? _IO_putc_unlocked(c, stream) : _IO_putc(c, stream))


__INLINE int
putc_unlocked(int c, FILE *stream)
{
	return _IO_putc_unlocked(c, stream);
}


__INLINE int
putchar_unlocked(int c)
{
	return _IO_putc_unlocked(c, stdout);
}


__INLINE int
getc_unlocked(FILE *stream)
{
	return _IO_getc_unlocked(stream);
}


__INLINE int
getchar_unlocked(void)
{
	return _IO_getc_unlocked(stdin);
}

#undef __INLINE

#endif	/* _STDIO_POST_H_ */
