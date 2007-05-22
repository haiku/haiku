/***********************************************************************
 *                                                                     *
 * $Id: hpgsistream.c 343 2006-08-18 16:48:09Z softadm $
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
 * The implementations of input streams.                               *
 *                                                                     *
 ***********************************************************************/

#include <hpgs.h>
#include <math.h>
#include <string.h>
#include <errno.h>

static int file_seek (FILE *file, size_t pos)
{
  return fseek(file,pos,SEEK_SET);
}

static int file_seekend (FILE *file, size_t pos)
{
  return fseek(file,-(long)pos,SEEK_END);
}

static int file_tell (FILE *file, size_t *pos)
{
  long pp=ftell(file);

  if (pp<0) return -1;

  *pos = pp;
  return 0;
}

/*! \defgroup base Basic facilities.

  This module contains structures for operating on abstract input streams
  as well as color structures and a 2D point.
*/

static hpgs_istream_vtable file_vtable =
  {
    (hpgs_istream_getc_func_t)    getc,
    (hpgs_istream_ungetc_func_t)  ungetc,
    (hpgs_istream_close_func_t)   fclose,
    (hpgs_istream_iseof_func_t)   feof,
    (hpgs_istream_iserror_func_t) ferror,
    (hpgs_istream_seek_func_t)    file_seek,
    (hpgs_istream_tell_func_t)    file_tell,
    (hpgs_istream_read_func_t)    fread,
    (hpgs_istream_seekend_func_t) file_seekend
  };

/*! Returns a new \c hpgs_istream created on the heap,
    which operates on a file, which is opened by this call
    in read-only mode.

    Returns a null pointer, when an I/O error occurrs.
    In this case, details about the the error can be retrieved
    using \c errno.
  */
hpgs_istream *hpgs_new_file_istream(const char *fn)
{
  FILE *in = fopen (fn,"rb");
  hpgs_istream *ret = 0;

  if (!in) return 0;

  ret = (hpgs_istream *)malloc(sizeof(hpgs_istream));

  if (!ret)
    {
      fclose(in);
      return 0;
    }

  ret->stream = in;
  ret->vtable = &file_vtable;

  return ret;
}

/* memory stream implementation functions. */
typedef struct hpgs_mem_istream_stream_st hpgs_mem_istream_stream;

struct hpgs_mem_istream_stream_st
{
  const unsigned char *data;
  const unsigned char *gptr;
  const unsigned char *eptr;
  int errflg;
};

static int mem_getc   (hpgs_mem_istream_stream *stream)
{
  int ret;

  if (stream->errflg)
    ret=EOF;
  else
    {
      if (stream->gptr >= stream->eptr)
        ret = EOF;
      else
        {
          ret = *stream->gptr;
          ++stream->gptr;
        }
    }

  return ret;
}

static int mem_ungetc (int c, hpgs_mem_istream_stream *stream)
{
  if (stream->errflg) return -1;
  if (stream->gptr <= stream->data) { stream->errflg = 1; return -1; }
  --stream->gptr;
  return 0;
}

static int mem_close  (hpgs_mem_istream_stream *stream)
{
  return 0;
}

static int mem_iseof  (hpgs_mem_istream_stream *stream)
{
  return stream->gptr >= stream->eptr;
}

static int mem_iserror  (hpgs_mem_istream_stream *stream)
{
  return stream->errflg;
}

static int mem_tell (hpgs_mem_istream_stream *stream, size_t *pos)
{
  if (stream->errflg) return -1;
  *pos = stream->gptr - stream->data;
  return 0;
}

static int mem_seek (hpgs_mem_istream_stream *stream, size_t pos)
{
  if (stream->data + pos > stream->eptr)
    {
      stream->errflg = 1;
      return -1;
    }

  stream->gptr = stream->data+pos;
  stream->errflg = 0;
  return 0;
}

static int mem_seekend (hpgs_mem_istream_stream *stream, size_t pos)
{
  if (stream->data + pos > stream->eptr)
    {
      stream->errflg = 1;
      return -1;
    }

  stream->gptr = stream->eptr-pos;
  stream->errflg = 0;
  return 0;
}

static size_t mem_read (void *ptr, size_t size, size_t nmemb, hpgs_mem_istream_stream *stream)
{
  size_t ret = 0;

  if (!stream->errflg)
    {
      if (stream->gptr < stream->eptr)
        {
          ret = (stream->eptr - stream->gptr)/size;
          if (ret > nmemb) ret = nmemb;

          memcpy(ptr,stream->gptr,size * ret);
          stream->gptr += size * ret;
        }
    }

  return ret;
}

static hpgs_istream_vtable mem_vtable =
  {
    (hpgs_istream_getc_func_t)    mem_getc,
    (hpgs_istream_ungetc_func_t)  mem_ungetc,
    (hpgs_istream_close_func_t)   mem_close,
    (hpgs_istream_iseof_func_t)   mem_iseof,
    (hpgs_istream_iserror_func_t) mem_iserror,
    (hpgs_istream_seek_func_t)    mem_seek,
    (hpgs_istream_tell_func_t)    mem_tell,
    (hpgs_istream_read_func_t)    mem_read,
    (hpgs_istream_seekend_func_t) mem_seekend
  };

/*! Returns a new \c hpgs_istream created on the heap,
    which operates on a chunk of memory in the given
    location with the given size.

    Returns a null pointer, when the system is out of memory.
  */
hpgs_istream *hpgs_new_mem_istream(const unsigned char *data,
				   size_t data_size,
                                   hpgs_bool dup)
{
#define HPGS_ISTREAM_PAD_SZ (8*((sizeof(hpgs_istream)+7)/8))
#define HPGS_ISTREAM_STREAM_PAD_SZ (8*((sizeof(hpgs_mem_istream_stream)+7)/8))

  hpgs_mem_istream_stream *stream;
  hpgs_istream *ret;
  size_t sz = HPGS_ISTREAM_PAD_SZ + HPGS_ISTREAM_STREAM_PAD_SZ;
  void *ptr;

  if (dup) sz += data_size;

  ptr = malloc(sz);
  ret = (hpgs_istream *)ptr;

  if (!ret)
    return 0;

  stream = (hpgs_mem_istream_stream *)(ptr+HPGS_ISTREAM_PAD_SZ);

  if (dup)
    {
      unsigned char *ddata =
        (unsigned char *)ptr + HPGS_ISTREAM_PAD_SZ + HPGS_ISTREAM_STREAM_PAD_SZ;

      stream->data = ddata;
      stream->gptr = ddata;
      stream->eptr = ddata+data_size;

      memcpy(ddata,data,data_size);
    }
  else
    {
      stream->data = data;
      stream->gptr = data;
      stream->eptr = data+data_size;
    }

  stream->errflg = 0;
    
  ret->stream = stream;
  ret->vtable = &mem_vtable;

  return ret;
}
