/***********************************************************************
 *                                                                     *
 * $Id: hpgszostream.c 368 2006-12-31 15:18:08Z softadm $
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
 * The implementation of a deflate output stream.                      *
 *                                                                     *
 ***********************************************************************/

#include<hpgs.h>
#include<string.h>
#include<zlib.h>
#ifdef WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

// Internals of hpgs_z_ostream
typedef struct hpgs_z_ostream_stream_st hpgs_z_ostream_stream;

#define Z_OSTREAM_BUF_SIZE 16384
#define Z_OSTREAM_XFER_SIZE   4096

struct hpgs_z_ostream_stream_st
{
  hpgs_ostream *base;

  int compression;
  hpgs_bool take_base;

  unsigned char buffer [Z_OSTREAM_BUF_SIZE];
  unsigned char xfer   [Z_OSTREAM_XFER_SIZE];

  unsigned char *pptr;

  int errflg;

  z_stream zstream;
  size_t total_bytes;
};

static int z_init(hpgs_z_ostream_stream *stream)
{
  if (deflateInit(&stream->zstream,stream->compression)==Z_OK)
    {
      stream->zstream.next_out = (Bytef *)stream->xfer;
      stream->zstream.avail_out = sizeof(stream->xfer);
      stream->zstream.total_out = 0;
      stream->zstream.avail_in = 0;
      stream->zstream.total_in = 0;
      stream->total_bytes = 0;
      return 0;
    }
  return -1;
}

static int z_consume(hpgs_z_ostream_stream *stream, int flags)
{
  int cnt=stream->pptr-stream->buffer;

  int zret;
  
  if (stream->errflg)
    return EOF;

  if (!stream->zstream.next_out && z_init(stream)) return EOF;

  if (cnt <= 0 && flags == Z_NO_FLUSH) return 0;

  stream->zstream.avail_in=cnt;
  stream->zstream.total_in=0;
  stream->zstream.next_in=(Bytef *)stream->buffer;

  do
    {
      stream->zstream.avail_out=Z_OSTREAM_XFER_SIZE;
      stream->zstream.total_out=0;
      stream->zstream.next_out=(Bytef *)stream->xfer;

      zret=deflate (&stream->zstream,flags);

      if (hpgs_ostream_write(stream->xfer,1,stream->zstream.total_out,stream->base) < stream->zstream.total_out)
        { stream->errflg = 1; return -1; }
    }

  while (zret == Z_OK &&
	 stream->zstream.avail_out == 0);

  stream->pptr = stream->buffer;


  if (stream->zstream.avail_in==0)
    {
      stream->total_bytes += cnt;
      return 0;
    }
  else
    {
      stream->errflg = 1;
      return -1;
    }
}

static int z_putc (int c, hpgs_z_ostream_stream *stream)
{
  if (stream->pptr >= stream->buffer + Z_OSTREAM_BUF_SIZE)
    z_consume(stream,Z_NO_FLUSH);

  if (stream->errflg)
    return EOF;

  *stream->pptr = c;
  ++stream->pptr;

  return c & 0xff;
}

static int z_write (void *ptr, size_t size, size_t nmemb, hpgs_z_ostream_stream *stream)
{
  size_t tot_bytes = size*nmemb;
  size_t avail_bytes = stream->buffer + Z_OSTREAM_BUF_SIZE - stream->pptr;
  size_t put_bytes = tot_bytes < avail_bytes ? tot_bytes : avail_bytes;

  if (stream->errflg)
    return EOF;

  memcpy(stream->pptr,ptr,put_bytes);
  stream->pptr += put_bytes;

  while (stream->pptr >= stream->buffer + Z_OSTREAM_BUF_SIZE &&
         z_consume(stream,Z_NO_FLUSH) == 0)
    {
      ptr += put_bytes;
      tot_bytes -= put_bytes;

      put_bytes = tot_bytes < Z_OSTREAM_BUF_SIZE ? tot_bytes : Z_OSTREAM_BUF_SIZE ;

      memcpy(stream->pptr,ptr,put_bytes);
      stream->pptr += put_bytes;
    }

  return stream->errflg ? 0 : nmemb;
}

static int z_vprintf (hpgs_z_ostream_stream *stream, const char *fmt, va_list ap)
{
  int n;
  size_t size=1024;
  char *buf;

  while (1)
    {
      if (stream->errflg)
        return EOF;

      buf = hpgs_alloca(size);

      /* Try to print in the allocated space. */
      va_list aq;
      va_copy(aq, ap);
#ifdef WIN32
      n = _vsnprintf (buf, size, fmt, aq);
#else
      n = vsnprintf (buf, size, fmt, aq);
#endif
      va_end(aq);

      /* If that worked, push the string. */
      if (n > -1 && n < size)
	return z_write(buf,n,1,stream) ? n : -1;
 
      /* Else try again with more space. */
      if (n < 0)
        size *= 2;
      else
        size = n+1;
    }

  return 0;
}

static int z_flush (hpgs_z_ostream_stream *stream)
{
  if (stream->zstream.next_out == 0 && stream->pptr <= stream->buffer) return 0;
  int ret = z_consume(stream,Z_FINISH);

  deflateEnd (&stream->zstream);
  stream->zstream.next_out = 0;

  return ret;
}

static int z_close (hpgs_z_ostream_stream *stream)
{
  int ret=0;

  if (stream->zstream.next_out)
    deflateEnd (&stream->zstream);

  if (stream->take_base && hpgs_ostream_close(stream->base))
    ret = -1;

  free(stream);

  return ret;
}

static int z_iserror (hpgs_z_ostream_stream *stream)
{
  return stream->errflg;
}

static hpgs_istream *z_getbuf (hpgs_z_ostream_stream *stream)
{
  z_flush(stream);
  return hpgs_ostream_getbuf(stream->base);
}

static int z_tell (hpgs_z_ostream_stream *stream, int layer, size_t *pos)
{
  if (layer > 0)
    return hpgs_ostream_tell(stream->base,layer-1,pos);
  else if (layer == 0)
    {
      *pos = stream->total_bytes;
      return 0;
    }
  else
    return -1;
}

static int z_seek (hpgs_z_ostream_stream *stream, size_t pos)
{
  // only seek(0) is supported in order to reuse the stream.
  if (pos) return -1;

  z_flush(stream);

  return hpgs_ostream_seek(stream->base,0);
}

static hpgs_ostream_vtable z_vtable =
  {
    (hpgs_ostream_putc_func_t)    z_putc,
    (hpgs_ostream_write_func_t)   z_write,
    (hpgs_ostream_vprintf_func_t) z_vprintf,
    (hpgs_ostream_flush_func_t)   z_flush,
    (hpgs_ostream_close_func_t)   z_close,
    (hpgs_ostream_iserror_func_t) z_iserror,
    (hpgs_ostream_getbuf_func_t)  z_getbuf,
    (hpgs_ostream_tell_func_t)    z_tell,
    (hpgs_ostream_seek_func_t)    z_seek,
  };

/*! Returns a new \c hpgs_ostream created on the heap,
    which operates on a given \c hpgs_ostream and writes
    the given data through a zlib deflate stream using
    the given compression.

    If \c take_base is false, the stream \c base is closed, when
    the returned stream is closed. In this case, don't close it at
    your own.

    Returns a null pointer, when the system is out of memory.
  */
hpgs_ostream *hpgs_new_z_ostream (hpgs_ostream *base, int compression, hpgs_bool take_base)
{
  hpgs_z_ostream_stream *stream;
  hpgs_ostream *ret = (hpgs_ostream *)malloc(sizeof(hpgs_ostream));

  if (!ret)
    return 0;

  stream =
    (hpgs_z_ostream_stream *)malloc(sizeof(hpgs_z_ostream_stream));

  if (!stream)
    {
      free (ret);
      return 0;
    }

  stream->base = base;
  stream->compression = compression;
  stream->take_base = take_base;

  stream->zstream.zalloc = Z_NULL;
  stream->zstream.zfree  = Z_NULL;
  stream->zstream.opaque = Z_NULL;

  stream->zstream.next_out = 0;
  stream->zstream.avail_out = 0;
  stream->zstream.total_out = 0;
  stream->zstream.avail_in = 0;
  stream->zstream.total_in = 0;
  stream->pptr = stream->buffer;
  stream->total_bytes = 0;
  stream->errflg = 0;
      
  ret->stream = stream;
  ret->vtable = &z_vtable;

  return ret;
}
