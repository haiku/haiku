/*
 * Copyright (c) 2011, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <fcntl.h>

#include "solv_xfopen.h"
#include "util.h"


static FILE *cookieopen(void *cookie, const char *mode,
	ssize_t (*cread)(void *, char *, size_t), 
	ssize_t (*cwrite)(void *, const char *, size_t), 
	int (*cclose)(void *))
{
#ifdef HAVE_FUNOPEN
  if (!cookie)
    return 0;
  return funopen(cookie, 
      (int (*)(void *, char *, int))(*mode == 'r' ? cread : NULL),		/* readfn */
      (int (*)(void *, const char *, int))(*mode == 'w' ? cwrite : NULL),	/* writefn */
      (fpos_t (*)(void *, fpos_t, int))NULL,					/* seekfn */
      cclose
      );
#elif defined(HAVE_FOPENCOOKIE)
  cookie_io_functions_t cio;

  if (!cookie)
    return 0;
  memset(&cio, 0, sizeof(cio));
  if (*mode == 'r')
    cio.read = cread;
  else if (*mode == 'w')
    cio.write = cwrite;
  cio.close = cclose;
  return  fopencookie(cookie, *mode == 'w' ? "w" : "r", cio);
#else
# error Need to implement custom I/O
#endif
}


/* gzip compression */

static ssize_t cookie_gzread(void *cookie, char *buf, size_t nbytes)
{
  return gzread((gzFile)cookie, buf, nbytes);
}

static ssize_t cookie_gzwrite(void *cookie, const char *buf, size_t nbytes)
{
  return gzwrite((gzFile)cookie, buf, nbytes);
}

static int cookie_gzclose(void *cookie)
{
  return gzclose((gzFile)cookie);
}

static inline FILE *mygzfopen(const char *fn, const char *mode)
{
  gzFile gzf = gzopen(fn, mode);
  return cookieopen(gzf, mode, cookie_gzread, cookie_gzwrite, cookie_gzclose);
}

static inline FILE *mygzfdopen(int fd, const char *mode)
{
  gzFile gzf = gzdopen(fd, mode);
  return cookieopen(gzf, mode, cookie_gzread, cookie_gzwrite, cookie_gzclose);
}

#ifdef ENABLE_BZIP2_COMPRESSION

#include <bzlib.h>

/* bzip2 compression */

static ssize_t cookie_bzread(void *cookie, char *buf, size_t nbytes)
{
  return BZ2_bzread((BZFILE *)cookie, buf, nbytes);
}

static ssize_t cookie_bzwrite(void *cookie, const char *buf, size_t nbytes)
{
  return BZ2_bzwrite((BZFILE *)cookie, (char *)buf, nbytes);
}

static int cookie_bzclose(void *cookie)
{
  BZ2_bzclose((BZFILE *)cookie);
  return 0;
}

static inline FILE *mybzfopen(const char *fn, const char *mode)
{
  BZFILE *bzf = BZ2_bzopen(fn, mode);
  return cookieopen(bzf, mode, cookie_bzread, cookie_bzwrite, cookie_bzclose);
}

static inline FILE *mybzfdopen(int fd, const char *mode)
{
  BZFILE *bzf = BZ2_bzdopen(fd, mode);
  return cookieopen(bzf, mode, cookie_bzread, cookie_bzwrite, cookie_bzclose);
}

#endif


#ifdef ENABLE_LZMA_COMPRESSION

#include <lzma.h>

/* lzma code written by me in 2008 for rpm's rpmio.c */

typedef struct lzfile {
  unsigned char buf[1 << 15];
  lzma_stream strm;
  FILE *file;
  int encoding;
  int eof;
} LZFILE;

static inline lzma_ret setup_alone_encoder(lzma_stream *strm, int level)
{
  lzma_options_lzma options;
  lzma_lzma_preset(&options, level);
  return lzma_alone_encoder(strm, &options);
}

static lzma_stream stream_init = LZMA_STREAM_INIT;

static LZFILE *lzopen(const char *path, const char *mode, int fd, int isxz)
{
  int level = 7;
  int encoding = 0;
  FILE *fp;
  LZFILE *lzfile;
  lzma_ret ret;

  if (!path && fd < 0)
    return 0;
  for (; *mode; mode++)
    {
      if (*mode == 'w')
	encoding = 1;
      else if (*mode == 'r')
	encoding = 0;
      else if (*mode >= '1' && *mode <= '9')
	level = *mode - '0';
    }
  if (fd != -1)
    fp = fdopen(fd, encoding ? "w" : "r");
  else
    fp = fopen(path, encoding ? "w" : "r");
  if (!fp)
    return 0;
  lzfile = calloc(1, sizeof(*lzfile));
  if (!lzfile)
    {
      fclose(fp);
      return 0;
    }
  lzfile->file = fp;
  lzfile->encoding = encoding;
  lzfile->eof = 0;
  lzfile->strm = stream_init;
  if (encoding)
    {
      if (isxz)
	ret = lzma_easy_encoder(&lzfile->strm, level, LZMA_CHECK_SHA256);
      else
	ret = setup_alone_encoder(&lzfile->strm, level);
    }
  else
    ret = lzma_auto_decoder(&lzfile->strm, 100 << 20, 0);
  if (ret != LZMA_OK)
    {
      fclose(fp);
      free(lzfile);
      return 0;
    }
  return lzfile;
}

static int lzclose(void *cookie)
{
  LZFILE *lzfile = cookie;
  lzma_ret ret;
  size_t n;
  int rc;

  if (!lzfile)
    return -1;
  if (lzfile->encoding)
    {
      for (;;)
	{
	  lzfile->strm.avail_out = sizeof(lzfile->buf);
	  lzfile->strm.next_out = lzfile->buf;
	  ret = lzma_code(&lzfile->strm, LZMA_FINISH);
	  if (ret != LZMA_OK && ret != LZMA_STREAM_END)
	    return -1;
	  n = sizeof(lzfile->buf) - lzfile->strm.avail_out;
	  if (n && fwrite(lzfile->buf, 1, n, lzfile->file) != n)
	    return -1;
	  if (ret == LZMA_STREAM_END)
	    break;
	}
    }
  lzma_end(&lzfile->strm);
  rc = fclose(lzfile->file);
  free(lzfile);
  return rc;
}

static ssize_t lzread(void *cookie, char *buf, size_t len)
{
  LZFILE *lzfile = cookie;
  lzma_ret ret;
  int eof = 0;

  if (!lzfile || lzfile->encoding)
    return -1;
  if (lzfile->eof)
    return 0;
  lzfile->strm.next_out = (unsigned char *)buf;
  lzfile->strm.avail_out = len;
  for (;;)
    {
      if (!lzfile->strm.avail_in)
	{
	  lzfile->strm.next_in = lzfile->buf;
	  lzfile->strm.avail_in = fread(lzfile->buf, 1, sizeof(lzfile->buf), lzfile->file);
	  if (!lzfile->strm.avail_in)
	    eof = 1;
	}
      ret = lzma_code(&lzfile->strm, LZMA_RUN);
      if (ret == LZMA_STREAM_END)
	{
	  lzfile->eof = 1;
	  return len - lzfile->strm.avail_out;
	}
      if (ret != LZMA_OK)
	return -1;
      if (!lzfile->strm.avail_out)
	return len;
      if (eof)
	return -1;
    }
}

static ssize_t lzwrite(void *cookie, const char *buf, size_t len)
{
  LZFILE *lzfile = cookie;
  lzma_ret ret;
  size_t n;
  if (!lzfile || !lzfile->encoding)
    return -1;
  if (!len)
    return 0;
  lzfile->strm.next_in = (unsigned char *)buf;
  lzfile->strm.avail_in = len;
  for (;;)
    {
      lzfile->strm.next_out = lzfile->buf;
      lzfile->strm.avail_out = sizeof(lzfile->buf);
      ret = lzma_code(&lzfile->strm, LZMA_RUN);
      if (ret != LZMA_OK)
	return -1;
      n = sizeof(lzfile->buf) - lzfile->strm.avail_out;
      if (n && fwrite(lzfile->buf, 1, n, lzfile->file) != n)
	return -1;
      if (!lzfile->strm.avail_in)
	return len;
    }
}

static inline FILE *myxzfopen(const char *fn, const char *mode)
{
  LZFILE *lzf = lzopen(fn, mode, -1, 1);
  return cookieopen(lzf, mode, lzread, lzwrite, lzclose);
}

static inline FILE *myxzfdopen(int fd, const char *mode)
{
  LZFILE *lzf = lzopen(0, mode, fd, 1);
  return cookieopen(lzf, mode, lzread, lzwrite, lzclose);
}

static inline FILE *mylzfopen(const char *fn, const char *mode)
{
  LZFILE *lzf = lzopen(fn, mode, -1, 0);
  return cookieopen(lzf, mode, lzread, lzwrite, lzclose);
}

static inline FILE *mylzfdopen(int fd, const char *mode)
{
  LZFILE *lzf = lzopen(0, mode, fd, 0);
  return cookieopen(lzf, mode, lzread, lzwrite, lzclose);
}

#endif /* ENABLE_LZMA_COMPRESSION */


FILE *
solv_xfopen(const char *fn, const char *mode)
{
  char *suf;

  if (!fn)
    return 0;
  if (!mode)
    mode = "r";
  suf = strrchr(fn, '.');
  if (suf && !strcmp(suf, ".gz"))
    return mygzfopen(fn, mode);
#ifdef ENABLE_LZMA_COMPRESSION
  if (suf && !strcmp(suf, ".xz"))
    return myxzfopen(fn, mode);
  if (suf && !strcmp(suf, ".lzma"))
    return mylzfopen(fn, mode);
#else
  if (suf && !strcmp(suf, ".xz"))
    return 0;
  if (suf && !strcmp(suf, ".lzma"))
    return 0;
#endif
#ifdef ENABLE_BZIP2_COMPRESSION
  if (suf && !strcmp(suf, ".bz2"))
    return mybzfopen(fn, mode);
#else
  if (suf && !strcmp(suf, ".bz2"))
    return 0;
#endif
  return fopen(fn, mode);
}

FILE *
solv_xfopen_fd(const char *fn, int fd, const char *mode)
{
  const char *simplemode = mode;
  char *suf;

  suf = fn ? strrchr(fn, '.') : 0;
  if (!mode)
    {
      int fl = fcntl(fd, F_GETFL, 0);
      if (fl == -1)
	return 0;
      fl &= O_RDONLY|O_WRONLY|O_RDWR;
      if (fl == O_WRONLY)
	mode = simplemode = "w";
      else if (fl == O_RDWR)
	{
	  mode = "r+";
	  simplemode = "r";
	}
      else
	mode = simplemode = "r";
    }
  if (suf && !strcmp(suf, ".gz"))
    return mygzfdopen(fd, simplemode);
#ifdef ENABLE_LZMA_COMPRESSION
  if (suf && !strcmp(suf, ".xz"))
    return myxzfdopen(fd, simplemode);
  if (suf && !strcmp(suf, ".lzma"))
    return mylzfdopen(fd, simplemode);
#else
  if (suf && !strcmp(suf, ".xz"))
    return 0;
  if (suf && !strcmp(suf, ".lzma"))
    return 0;
#endif
#ifdef ENABLE_BZIP2_COMPRESSION
  if (suf && !strcmp(suf, ".bz2"))
    return mybzfdopen(fd, simplemode);
#else
  if (suf && !strcmp(suf, ".bz2"))
    return 0;
#endif
  return fdopen(fd, mode);
}

int
solv_xfopen_iscompressed(const char *fn)
{
  const char *suf = fn ? strrchr(fn, '.') : 0;
  if (!suf)
    return 0;
  if (!strcmp(suf, ".gz"))
    return 1;
  if (!strcmp(suf, ".xz") || !strcmp(suf, ".lzma"))
#ifdef ENABLE_LZMA_COMPRESSION
    return 1;
#else
    return -1;
#endif
  if (!strcmp(suf, ".bz2"))
#ifdef ENABLE_BZIP2_COMPRESSION
    return 1;
#else
    return -1;
#endif
  return 0;
}

struct bufcookie {
  char **bufp;
  size_t *buflp;
  char *freemem;
  size_t bufl_int;
};

static ssize_t cookie_bufread(void *cookie, char *buf, size_t nbytes)
{
  struct bufcookie *bc = cookie;
  size_t n = *bc->buflp > nbytes ? nbytes : *bc->buflp;
  if (n)
    {
      memcpy(buf, *bc->bufp, n);
      *bc->bufp += n;
      *bc->buflp -= n;
    }
  return n;
}

static ssize_t cookie_bufwrite(void *cookie, const char *buf, size_t nbytes)
{
  struct bufcookie *bc = cookie;
  int n = nbytes > 0x40000000 ? 0x40000000 : nbytes;
  if (n)
    {
      *bc->bufp = solv_extend(*bc->bufp, *bc->buflp, n + 1, 1, 4095);
      memcpy(*bc->bufp, buf, n);
      (*bc->bufp)[n] = 0;	/* zero-terminate */
      *bc->buflp += n;
    }
  return n;
}

static int cookie_bufclose(void *cookie)
{
  struct bufcookie *bc = cookie;
  if (bc->freemem)
    solv_free(bc->freemem);
  solv_free(bc);
  return 0;
}

FILE *
solv_xfopen_buf(const char *fn, char **bufp, size_t *buflp, const char *mode)
{
  struct bufcookie *bc;
  FILE *fp;
  if (*mode != 'r' && *mode != 'w')
    return 0;
  bc = solv_calloc(1, sizeof(*bc));
  bc->freemem = 0;
  bc->bufp = bufp;
  if (!buflp)
    {
      bc->bufl_int = *mode == 'w' ? 0 : strlen(*bufp);
      buflp = &bc->bufl_int;
    }
  bc->buflp = buflp;
  if (*mode == 'w')
    {
      *bc->bufp = solv_extend(0, 0, 1, 1, 4095);	/* always zero-terminate */
      (*bc->bufp)[0] = 0;
      *bc->buflp = 0;
    }
  fp = cookieopen(bc, mode, cookie_bufread, cookie_bufwrite, cookie_bufclose);
  if (!strcmp(mode, "rf"))	/* auto-free */
    bc->freemem = *bufp;
  if (!fp)
    {
      if (*mode == 'w')
	*bc->bufp = solv_free(*bc->bufp);
      cookie_bufclose(bc);
    }
  return fp;
}
