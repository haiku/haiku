/***********************************************************************
 *                                                                     *
 * $Id: hpgsostream.c 368 2006-12-31 15:18:08Z softadm $
 *                                                                     *
 * hpgs - HPGl Script, a hpgl/2 interpreter, which uses a Postscript   *
 *        API for rendering a scene and thus renders to a variety of   *
 *        devices and fileformats.                                     *
 *                                                                     *
 * (C) 2004-2006 ev-i Informationstechnologie GmbH  http://www.ev-i.at *
 *                                                                     *
 * Author: Wolfgang Glas                                               *
 *                                                                     *
 *  hpgs is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * hpgs is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the               *
 * Free Software  Foundation, Inc., 59 Temple Place, Suite 330,        *
 * Boston, MA  02111-1307  USA                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * The implementation of basic output streams.                         *
 *                                                                     *
 ***********************************************************************/

#include<hpgs.h>
#include<string.h>
#include<stdarg.h>

/*! The counterpart of ANSI fprintf for \c hpgs_ostream. */
int hpgs_ostream_printf (hpgs_ostream *_this, const char *msg, ...)
{
  int ret;
  va_list ap;
  va_start(ap,msg);
  ret=_this->vtable->vprintf_func(_this->stream,msg,ap);
  va_end(ap);
  return ret;
}

/*! The counterpart of ANSI fvprintf for \c hpgs_ostream. */
int hpgs_ostream_vprintf (hpgs_ostream *_this, const char *msg, va_list ap)
{
  int ret;
  va_list aq;
  va_copy(aq, ap);
  ret = _this->vtable->vprintf_func(_this->stream,msg,aq);
  va_end(aq);
  return ret;
}


static int file_seek (FILE *file, size_t pos)
{
  return fseek(file,pos,SEEK_SET);
}

static int file_tell (FILE *file, int layer, size_t *pos)
{
  if (layer) return -1;

  long pp=ftell(file);

  if (pp<0) return -1;

  *pos = pp;
  return 0;
}

static hpgs_ostream_vtable stdfile_vtable =
  {
    (hpgs_ostream_putc_func_t)    putc,
    (hpgs_ostream_write_func_t)   fwrite,
    (hpgs_ostream_vprintf_func_t) vfprintf,
    (hpgs_ostream_flush_func_t)   fflush,
    (hpgs_ostream_close_func_t)   fflush,
    (hpgs_ostream_iserror_func_t) ferror,
    (hpgs_ostream_getbuf_func_t)  0,
    (hpgs_ostream_tell_func_t)    file_tell,
    (hpgs_ostream_seek_func_t)    0
  };

static hpgs_ostream_vtable file_vtable =
  {
    (hpgs_ostream_putc_func_t)    putc,
    (hpgs_ostream_write_func_t)   fwrite,
    (hpgs_ostream_vprintf_func_t) vfprintf,
    (hpgs_ostream_flush_func_t)   fflush,
    (hpgs_ostream_close_func_t)   fclose,
    (hpgs_ostream_iserror_func_t) ferror,
    (hpgs_ostream_getbuf_func_t)  0,
    (hpgs_ostream_tell_func_t)    file_tell,
    (hpgs_ostream_seek_func_t)    file_seek
  };

/*! Returns a new \c hpgs_ostream created on the heap,
    which operates on a file, which is opened by this call
    in write-only mode.

    Returns a null pointer, when an I/O error occurrs.
    In this case, details about the the error can be retrieved
    using \c errno.
  */
hpgs_ostream *hpgs_new_file_ostream (const char *fn)
{
  FILE *out = fn ? fopen (fn,"wb") : stdout;
  hpgs_ostream *ret = 0;

  if (!out) return 0;

  ret = (hpgs_ostream *)malloc(sizeof(hpgs_ostream));

  if (!ret)
    {
      fclose(out);
      return 0;
    }

  ret->stream = out;

  if (fn)
    ret->vtable = &file_vtable;
  else
    ret->vtable = &stdfile_vtable;

  return ret;
}

typedef struct hpgs_mem_ostream_stream_st hpgs_mem_ostream_stream;

struct hpgs_mem_ostream_stream_st
{
  unsigned char *data;
  unsigned char *pptr;
  unsigned char *eptr;
  int errflg;
};

static int mem_grow (hpgs_mem_ostream_stream *stream, size_t hint)
{
  if (stream->errflg)
    return EOF;

  size_t pos = stream->pptr-stream->data;
  size_t sz  = stream->eptr-stream->data;
  size_t sz2 = (hint > sz) ? sz+hint : 2*sz;

  // check for overflow.
  if (sz2 < sz)
    {
      stream->errflg = 1;
      return EOF;
    }

  unsigned char *ndata=(unsigned char *)realloc(stream->data,sz2);
  
  if (ndata)
    {
      stream->data=ndata;
      stream->pptr=stream->data+pos;
      stream->eptr=stream->data+sz2;
      return 0;
    }
  else
    {
      stream->errflg = 1;
      return EOF;
    }
}

static int mem_putc (int c, hpgs_mem_ostream_stream *stream)
{
  if (stream->pptr >= stream->eptr)
    mem_grow(stream,0);

  if (stream->errflg)
    return EOF;

  *stream->pptr = c;
  ++stream->pptr;

  return c & 0xff;
}

static size_t mem_write (void *ptr, size_t size, size_t nmemb, hpgs_mem_ostream_stream *stream)
{
  if (stream->pptr + size*nmemb > stream->eptr)
    mem_grow(stream,size*nmemb);

  if (stream->errflg)
    return 0;

  memcpy(stream->pptr,ptr,size*nmemb);
  stream->pptr += size*nmemb;

  return nmemb;
}

static int mem_vprintf (hpgs_mem_ostream_stream *stream, const char *fmt, va_list ap)
{
  int n;
  size_t size;

  while (1)
    {
      if (stream->errflg)
        return EOF;

      size = stream->eptr - stream->pptr;
      /* Try to print in the allocated space. */
      va_list aq;
      va_copy(aq, ap);
#ifdef WIN32
      n = _vsnprintf (stream->pptr, size, fmt, aq);
#else
      n = vsnprintf ((char*)stream->pptr, size, fmt, aq);
#endif
      va_end(aq);
      /* If that worked, return the string. */
      if (n > -1 && n < size)
	{
          stream->pptr += n;
          return n;
        }
 
      /* Else try again with more space. */
      mem_grow(stream,0);
    }

  return 0;
}

static int mem_close (hpgs_mem_ostream_stream *stream)
{
  if (stream->data) free(stream->data);
  free(stream);
  return 0;
}

static int mem_iserror (hpgs_mem_ostream_stream *stream)
{
  return stream->errflg;
}

static hpgs_istream *mem_getbuf (hpgs_mem_ostream_stream *stream)
{
  if (stream->errflg || !stream->data)
    return 0;

  return hpgs_new_mem_istream (stream->data,stream->pptr-stream->data,HPGS_FALSE);
}

static int mem_tell (hpgs_mem_ostream_stream *stream, int layer, size_t *pos)
{
  if (layer || stream->errflg) return -1;

  *pos = stream->pptr-stream->data;
  return 0;
}

static int mem_seek (hpgs_mem_ostream_stream *stream, size_t pos)
{
  if (stream->errflg) return -1;

  if (stream->data + pos > stream->pptr)
    {
      stream->errflg = 1;
      return -1;
    }

  stream->pptr = stream->data + pos;
  return 0;
}

static hpgs_ostream_vtable mem_vtable =
  {
    (hpgs_ostream_putc_func_t)    mem_putc,
    (hpgs_ostream_write_func_t)   mem_write,
    (hpgs_ostream_vprintf_func_t) mem_vprintf,
    (hpgs_ostream_flush_func_t)   0,
    (hpgs_ostream_close_func_t)   mem_close,
    (hpgs_ostream_iserror_func_t) mem_iserror,
    (hpgs_ostream_getbuf_func_t)  mem_getbuf,
    (hpgs_ostream_tell_func_t)    mem_tell,
    (hpgs_ostream_seek_func_t)    mem_seek
  };

/*! Returns a new \c hpgs_ostream created on the heap,
    which operates on a malloced chunk of memory with
    the preallocated given size.

    The memory buffer is realloced when the data grows over the
    given preallocated size.

    Returns a null pointer, when the system is out of memory.
  */
hpgs_ostream *hpgs_new_mem_ostream (size_t data_reserve)
{
  hpgs_mem_ostream_stream *stream;
  hpgs_ostream *ret = (hpgs_ostream *)malloc(sizeof(hpgs_ostream));

  if (!ret)
    return 0;

  stream =
    (hpgs_mem_ostream_stream *)malloc(sizeof(hpgs_mem_ostream_stream));

  if (!stream)
    {
      free (ret);
      return 0;
    }

  stream->data = (unsigned char *)malloc(data_reserve);
  stream->pptr = stream->data;
  stream->eptr =  stream->data ? stream->data+data_reserve : 0;
  stream->errflg = stream->data ? 0 : 1;

  ret->stream = stream;
  ret->vtable = &mem_vtable;

  return ret;
}

int hpgs_copy_streams (hpgs_ostream *out, hpgs_istream *in)
{
  unsigned char buf[16384];
  size_t sz;
  
  while ((sz = hpgs_istream_read(buf,1,sizeof(buf),in)) > 0)
    if (hpgs_ostream_write(buf,1,sz,out) == 0)
      return -1;

  return hpgs_istream_iserror(in);
}
