/* stdio.h
 */

#ifndef _STDIO_H
#define _STDIO_H

#include <ktypes.h>
#include <cdefs.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef off_t fpos_t;  /* stdio file position type */

/* stdio buffers */
struct __sbuf {
        unsigned char *_base;
        int     _size;
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

/*
 * FOPEN_MAX is a minimum maximum, and should be the number of descriptors
 * that the kernel can provide without allocation of a resource that can
 * fail without the process sleeping.  Do not use this for anything
 */
#define FOPEN_MAX       20      /* must be <= OPEN_MAX <sys/syslimits.h> */ 
#define BUFSIZ        1024

extern FILE __sF[];

#define stdin  (&__sF[0])
#define stdout (&__sF[1])
#define stderr (&__sF[2])

//extern FILE *stdout;
//extern FILE *stderr;

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

#define EOF -1

int printf(char const *format, ...) __PRINTFLIKE(1,2);
int fprintf(FILE *stream, char const *format, ...) __PRINTFLIKE(2,3);
int sprintf(char *str, char const *format, ...) __PRINTFLIKE(2,3);
int snprintf(char *str, size_t size, char const *format, ...) __PRINTFLIKE(3,4);
int asprintf(char **ret, char const *format, ...) __PRINTFLIKE(2,3);
int vprintf(char const *format, va_list ap);
int vfprintf(FILE *stream, char const *format, va_list ap);
int vsprintf(char *str, char const *format, va_list ap);
int vsnprintf(char *str, size_t size, char const *format, va_list ap);
int vasprintf(char **ret, char const *format, va_list ap);

void   clearerr(FILE *);
int    fclose(FILE *);
int    feof(FILE *);
int    fflush(FILE *);
int    fgetc(FILE *);
char  *fgetln(FILE *, size_t *);
int    fgetpos(FILE *, fpos_t);
char  *fgets(char *, int, FILE *);
void   flockfile(FILE *);
FILE  *fopen(char const *, char const *);
int    fputc(int, FILE *);
int    fputs(const char *, FILE*);
size_t fread(void *, size_t, size_t, FILE *);
int    fscanf(FILE *stream, char const *format, ...);
int    fseek(FILE *, long, int);
int    fseeko(FILE *, off_t, int);
size_t fwrite(const void *, size_t, size_t, FILE *);
void   funlockfile(FILE *);
int    getc(FILE *);
char  *gets(char *);
int    getw(FILE *);
int    getchar(void);
int    putc(int, FILE *);
int    putchar(int);
int    puts(const char *);
int    putw(int, FILE *);
void   rewind(FILE *);
int    scanf(char const *format, ...);
int    sscanf(char const *str, char const *format, ...);
int    ungetc(int, FILE *);
int    vscanf(char const *format, va_list ap);
int    vsscanf(char const *str, char const *format, va_list ap);
int    vfscanf(FILE *stream, char const *format, va_list ap);


int   __srget(FILE *);
int   __svfscanf(FILE *, char const *, va_list ap);
int	  __swbuf(int c, FILE* fp);

#define __sgetc(p)    (--(p)->_r < 0 ? __srget(p) : (int)(*(p)->_p++))
static __inline int __sputc(int _c, FILE *_p) { 
	if (--_p->_w >= 0 || (_p->_w >= _p->_lbfsize && (char)_c != '\n'))
		return (*_p->_p++ = _c);
	else
		return (__swbuf(_c, _p));
}

#define __sfeof(x)    (((x)->_flags & __SEOF) != 0)
#define __sferror(x)  (((x)->_flags & __SERR) != 0)

#define feof(x)      __sfeof(x)
#define ferror(x)    __sferror(x)

#ifdef __cplusplus
}
#endif

#endif /* _STDIO_H */
