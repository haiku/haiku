#ifndef _STDIO_POST_H_
#define _STDIO_POST_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/


// "Private"/inline functions of our BeOS compatible stdio implementation

// ToDo: this is a work in progress to make our stdio
//		BeOS' GNU/libio (almost) binary compatible
//		We are not yet compatible!
//		Currently only function names are compatible

#ifndef _STDIO_H_
#	error "This file must be included from stdio.h!"
#endif

#ifdef __cplusplus
#	define __INLINE inline
#else
#	define __INLINE extern __inline
#endif


extern int _IO_feof_unlocked(FILE *stream);
extern int _IO_ferror_unlocked(FILE *stream);
extern int _IO_putc(int c, FILE *stream);
extern int _IO_putc_unlocked(int c, FILE *stream);
extern int _IO_getc(FILE *stream);
extern int _IO_getc_unlocked(FILE *stream);
extern int _IO_peekc_unlocked(FILE *stream);

extern int __underflow(FILE *stream);
extern int __uflow(FILE *stream);
extern int __overflow(FILE *stream, int c);


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
_IO_getc_unlocked(FILE *stream)
{
	if (stream->_IO_read_ptr >= stream->_IO_read_end)
		return __uflow(stream);
	
	return *(unsigned char *)stream->_IO_read_ptr++;
}


__INLINE int
_IO_peekc_unlocked(FILE *stream)
{
	if (stream->_IO_read_ptr >= stream->_IO_read_end && __underflow(stream) == EOF)
		return EOF;
	
	return *(unsigned char *)stream->_IO_read_ptr;
}


__INLINE int
_IO_putc_unlocked(int c, FILE *stream)
{
	if (stream->_IO_write_ptr >= stream->_IO_write_end)
		return __overflow(stream, (unsigned char)c);

	return (unsigned char)(*stream->_IO_write_ptr++ = c);
}


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

#undef __INLINE

#endif	/* _STDIO_POST_H_ */
