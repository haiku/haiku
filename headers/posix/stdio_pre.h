#ifndef _STDIO_PRE_H_
#define _STDIO_PRE_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/


// Details of the BeOS compatible stdio implementation
// Structure definitions

// ToDo: this is a work in progress to make our stdio
//		BeOS' GNU/libio (almost) binary compatible
//		We are not yet compatible!
//		Currently only function names are compatible

#ifndef _STDIO_H_
#	error "This file must be included from stdio.h!"
#endif


extern char *_single_threaded;
	/* This determines if a process runs single threaded or not */


/* stdio buffers */
struct __sbuf {
	unsigned char *_base;
	int		_size;
};

/*
 * stdio state variables.
 *
 * The following always hold:
 *
 *if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *              _lbfsize is -_bf._size, else _lbfsize is 0
 *if _flags&__SRD, _w is 0
 *if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.
 */ 
typedef struct __FILE {
	// ToDo: these fields must be maintained (at the same position)!
	//int _flags;				/* High-order word is _IO_MAGIC; rest is flags. */
	char *_IO_read_ptr;		/* Current read pointer */
	char *_IO_read_end;		/* End of get area. */
	//char *_IO_read_base;	/* Start of putback+get area. */
	//char *_IO_write_base;	/* Start of put area. */
	char *_IO_write_ptr;	/* Current put pointer. */
	char *_IO_write_end;	/* End of put area. */

	unsigned char *_p;/* current position in (some) buffer */
	int     _r;       /* read space left for getc() */
	int     _w;       /* write space left for putc() */
	short   _flags;   /* flags, below; this FILE is free if 0 */
	short   _file;    /* fileno, if Unix descriptor, else -1 */
	struct  __sbuf _bf;     /* the buffer (at least 1 byte, if !NULL) */
	int     _lbfsize;/* 0 or -_bf._size, for inline putc */

	/* operations */
	void    *_cookie;/* cookie passed to io functions */
	int     (*_close) (void *);
	int     (*_read)  (void *, char *, int);
	fpos_t  (*_seek)  (void *, fpos_t, int);
	int     (*_write) (void *, const char *, int);

	/* separate buffer for long sequences of ungetc() */
	struct  __sbuf _ub;     /* ungetc buffer */
	unsigned char *_up;     /* saved _p when _p is doing ungetc data */
	int     _ur;/* saved _r when _r is counting ungetc data */

	/* tricks to meet minimum requirements even when malloc() fails */
	unsigned char _ubuf[3]; /* guarantee an ungetc() buffer */
	unsigned char _nbuf[1]; /* guarantee a getc() buffer */

	/* separate buffer for fgetln() when line crosses buffer boundary */
	struct  __sbuf _lb;     /* buffer for fgetln() */

	/* Unix stdio files get aligned to block boundaries on fseek() */
	int     _blksize;/* stat.st_blksize (may be != _bf._size) */
	fpos_t  _offset;/* current lseek offset */

	// ToDo: add structure reserved fields
} FILE;  

#define __SLBF  0x0001    /* line buffered */
#define __SNBF  0x0002    /* unbuffered */
#define __SRD   0x0004    /* OK to read */
#define __SWR   0x0008    /* OK to write */
  /* RD and WR are never simultaneously asserted */
#define __SRW   0x0010    /* open for reading & writing */
#define __SEOF  0x0020    /* found EOF */
#define __SERR  0x0040    /* found error */
#define __SMBF  0x0080    /* _buf is from malloc */
#define __SAPP  0x0100    /* fdopen()ed in append mode */
#define __SSTR  0x0200    /* this is an sprintf/snprintf string */
#define __SOPT  0x0400    /* do fseek() optimisation */
#define __SNPT  0x0800    /* do not do fseek() optimisation */
#define __SOFF  0x1000    /* set iff _offset is in fact correct */
#define __SMOD  0x2000    /* true => fgetln modified _p text */
#define __SALC  0x4000    /* allocate string space dynamically */  

#define __PRINTFLIKE(format, varargs) __attribute__ ((__format__ (__printf__, format, varargs)));
#define __SCANFLIKE(format, varargs) __attribute__((__format__ (__scanf__, format, varargs)))

#endif	/* _STDIO_PRE_H_ */
