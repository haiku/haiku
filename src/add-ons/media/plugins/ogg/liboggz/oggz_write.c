/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"

#if OGGZ_CONFIG_WRITE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <ogg/ogg.h>

#include "oggz_private.h"
#include "oggz_vector.h"

/* #define DEBUG */

/* #define ALWAYS_FLUSH */

/* #define ZPACKET_CMP */

#ifdef ZPACKET_CMP
static int
oggz_zpacket_cmp (oggz_writer_packet_t * a, oggz_writer_packet_t * b,
		  void * user_data)
{
  OGGZ * oggz = (OGGZ *)user_data;
  long serialno_a, serialno_b;
  ogg_int64_t unit_a, unit_b;

  serialno_a = a->stream->ogg_stream.serialno;
  serialno_b = b->stream->ogg_stream.serialno;

  unit_a = oggz_get_unit (oggz, serialno_a, a->op.granulepos);
  unit_b = oggz_get_unit (oggz, serialno_b, b->op.granulepos);

  if (unit_a < unit_b) return -1;
  else return (unit_a > unit_b);
}
#endif

OGGZ *
oggz_write_init (OGGZ * oggz)
{
  OggzWriter * writer = &oggz->x.writer;

  writer->next_zpacket = NULL;

  writer->packet_queue = oggz_vector_new ();

#ifdef ZPACKET_CMP
  /* XXX: comparison function should only kick in when a metric is set */
  oggz_vector_set_cmp (writer->packet_queue, 
		       (OggzCmpFunc)oggz_zpacket_cmp, oggz);
#endif

  writer->hungry = NULL;
  writer->hungry_user_data = NULL;
  writer->hungry_only_when_empty = 0;

  writer->writing = 0;
  writer->state = OGGZ_MAKING_PACKETS;

  writer->flushing = 0;
#if 0
  writer->eog = 1;
  writer->eop = 1; /* init ready to start next packet */
#endif
  writer->eos = 0;

  writer->current_zpacket = NULL;

  writer->packet_offset = 0;
  writer->page_offset = 0;

  return oggz;
}

static int
oggz_writer_packet_free (oggz_writer_packet_t * zpacket)
{
  if (!zpacket) return 0;

  if (zpacket->guard) {
    /* managed by user; flag guard */
    *zpacket->guard = 1;
  } else {
    /* managed by oggz; free copied data */
    oggz_free (zpacket->op.packet);
  }
  oggz_free (zpacket);

  return 0;
}

OGGZ *
oggz_write_close (OGGZ * oggz)
{
  OggzWriter * writer = &oggz->x.writer;

  oggz_vector_foreach (writer->packet_queue,
		       (OggzFunc)oggz_writer_packet_free);
  oggz_vector_delete (writer->packet_queue);

  return oggz;
}

/******** Packet queueing ********/

int
oggz_write_set_hungry_callback (OGGZ * oggz, OggzWriteHungry hungry,
				int only_when_empty, void * user_data)
{
  OggzWriter * writer;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (!(oggz->flags & OGGZ_WRITE)) {
    return OGGZ_ERR_INVALID;
  }

  writer = &oggz->x.writer;

  writer->hungry = hungry;
  writer->hungry_user_data = user_data;
  writer->hungry_only_when_empty = only_when_empty;

  return 0;
}

int
oggz_write_feed (OGGZ * oggz, ogg_packet * op, long serialno, int flush,
		 int * guard)
{
  OggzWriter * writer;
  oggz_stream_t * stream;
  oggz_writer_packet_t * packet;
  ogg_packet * new_op;
  unsigned char * new_buf;
  int b_o_s, e_o_s, bos_auto;
  int strict;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (!(oggz->flags & OGGZ_WRITE)) {
    return OGGZ_ERR_INVALID;
  }

  writer = &oggz->x.writer;

  if (guard && *guard != 0) return OGGZ_ERR_BAD_GUARD;

#ifdef DEBUG
  printf ("oggz_write_feed: (%010ld) FLUSH: %d\n", serialno, flush);
#endif

  /* Cache strict */
  strict = !(oggz->flags & OGGZ_NONSTRICT);

  /* Set bos, eos to canonical values */
  bos_auto = (op->b_o_s == -1) ? 1 : 0;
  b_o_s = op->b_o_s ? 1 : 0;
  e_o_s = op->e_o_s ? 1 : 0;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) {
    if (bos_auto) b_o_s = 1;

    if (strict && b_o_s && !oggz_get_bos (oggz, -1)) {
	return OGGZ_ERR_BOS;
    }

    if (b_o_s || !strict) {
      stream = oggz_add_stream (oggz, serialno);
    } else {
      return OGGZ_ERR_BAD_SERIALNO;
    }
  } else {
    if (bos_auto) b_o_s = 0;

    if (strict && stream->e_o_s)
      return OGGZ_ERR_EOS;
  }

  /* Check packet against mapping restrictions */
  if (strict) {
    if (op->bytes < 0) return OGGZ_ERR_BAD_BYTES;
    if (b_o_s != stream->b_o_s) return OGGZ_ERR_BAD_B_O_S;

    if (op->granulepos != -1 && op->granulepos < stream->granulepos)
      return OGGZ_ERR_BAD_GRANULEPOS;
    
    /* Allow packetno == -1 to indicate oggz should fill it in; otherwise:
     * if op is the first packet in the stream, initialize the stream's
     * packetno to the given one, otherwise check it for strictness */
    if (op->packetno != -1) {
      if (b_o_s) {
	stream->packetno = op->packetno;
      } else if (op->packetno <= stream->packetno) {
	return OGGZ_ERR_BAD_PACKETNO;
      }
    }
  }

  /* OK -- Update stream's memory of packet details */

  stream->b_o_s = 0; /* The stream is henceforth no longer at bos */
  stream->e_o_s = e_o_s; /* We believe the eos value */
  stream->granulepos = op->granulepos; /* and the granulepos */

  /* If the user specified a packetno of -1, fill it in automatically;
   * otherwise, use the user-specified value */
  stream->packetno = (op->packetno != -1) ? op->packetno : stream->packetno+1;

  /* Now set up the packet and add it to the queue */
  if (guard == NULL) {
    new_buf = oggz_malloc ((size_t)op->bytes);
    memcpy (new_buf, op->packet, (size_t)op->bytes);
  } else {
    new_buf = op->packet;
  }

  packet = oggz_malloc (sizeof (oggz_writer_packet_t));

  new_op = &packet->op;
  new_op->packet = new_buf;
  new_op->bytes = op->bytes;
  new_op->b_o_s = op->b_o_s;
  new_op->e_o_s = op->e_o_s;
  new_op->granulepos = op->granulepos;
  new_op->packetno = stream->packetno;

  packet->stream = stream;
  packet->flush = flush;
  packet->guard = guard;

#ifdef DEBUG
  printf ("oggz_write_feed: made packet bos %ld eos %ld (%ld bytes) FLUSH: %d\n",
	  new_op->b_o_s, new_op->e_o_s, new_op->bytes, packet->flush);
#endif

  if (oggz_vector_insert_p (writer->packet_queue, packet) == NULL) {
    oggz_free (packet);
    if (!guard) oggz_free (new_buf);
    return -1;
  }

  return 0;
}

/******** Page creation ********/

/*
 *

            ____           __________________\___           ____
           /    \         /    Page ready    /   \         /    \
          |      \ /=========\                /=========\ /      |
  Page    |       || Make    ||              ||  Write  ||       | Page
  !ready  V       || packet  ||              ||  page   ||       V ready
          |      / \=========/                \=========/ \      |
           \____/         \___/__________________/         \____/
                              \   Page !ready
 *
 */


/*
 * oggz_page_init (oggz)
 *
 * Initialises the next page of the current packet.
 *
 * If this returns 0, the page is not ready for writing.
 */
static long
oggz_page_init (OGGZ * oggz)
{
  OggzWriter * writer = &oggz->x.writer;
  ogg_stream_state * os;
  ogg_page * og;
  int ret;

  if (oggz == NULL) return -1;

  os = writer->current_stream;
  og = &oggz->current_page;

#ifdef ALWAYS_FLUSH
  ret = ogg_stream_flush (os, og);
#else
  if (writer->flushing) {
#ifdef DEBUG
    printf ("*** ATTEMPT FLUSH: ");
#endif
    ret = ogg_stream_flush (os, og);
  } else {
#ifdef DEBUG
    printf ("*** ATTEMPT pageout: ");
#endif
    ret = ogg_stream_pageout (os, og);
  }
#endif

  if (ret) {
    writer->page_offset = 0;
  }

#ifdef DEBUG
  printf ("%s\n", ret ? "OK" : "NO");
#endif

  return ret;
}

/*
 * oggz_packet_init (oggz, buf, n)
 *
 * Initialises the next packet with data from buf, length n
 */
static long
oggz_packet_init (OGGZ * oggz, oggz_writer_packet_t * next_zpacket)
{
  OggzWriter * writer = &oggz->x.writer;
  oggz_stream_t * stream;
  ogg_stream_state * os;
  ogg_packet * op;

  if (oggz == NULL) return -1L;

  writer->current_zpacket = next_zpacket;
  op = &next_zpacket->op;

#ifdef DEBUG
  printf ("oggz_packet_init: %ld bytes\n", (long)op->bytes);
#endif

  stream = next_zpacket->stream;

  /* Mark this stream as having delivered a non b_o_s packet if so */
  if (!op->b_o_s) stream->delivered_non_b_o_s = 1;

  os = &stream->ogg_stream;
  ogg_stream_packetin (os, op);

  writer->flushing = (next_zpacket->flush & OGGZ_FLUSH_AFTER);
#ifdef DEBUG
  printf ("oggz_packet_init: set flush to %d\n", writer->flushing);
#endif
  writer->current_stream = os;
  writer->packet_offset = 0;

  return 0;
}

static long
oggz_page_copyout (OGGZ * oggz, unsigned char * buf, long n)
{
  OggzWriter * writer = &oggz->x.writer;
  long h, b;
  ogg_page * og;

  if (oggz == NULL) return -1L;

  og = &oggz->current_page;

  h = MIN (n, og->header_len - writer->page_offset);
  if (h > 0) {
    memcpy (buf, og->header + writer->page_offset, h);
    writer->page_offset += h;
    n -= h;
    buf += h;
  } else {
    h = 0;
  }
  
  b = MIN (n, og->header_len + og->body_len - writer->page_offset);
  if (b > 0) {
#ifdef DEBUG
    {
      unsigned char * c = &og->body[writer->page_offset - og->header_len];
      printf ("oggz_page_copyout [%d] (@%ld): %c%c%c%c ...\n",
	      ogg_page_serialno (og), (long) ogg_page_granulepos (og),
	      c[0], c[1], c[2], c[3]);
    }
#endif
    memcpy (buf, og->body + (writer->page_offset - og->header_len), b);
    writer->page_offset += b;
    n -= b;
    buf += b;
  } else {
    b = 0;
  }

  return h + b;
}

static long
oggz_page_writeout (OGGZ * oggz, long n)
{
  OggzWriter * writer = &oggz->x.writer;
  long h, b, nwritten;
  ogg_page * og;

#ifdef OGGZ_WRITE_DIRECT
  int fd;
#endif

  if (oggz == NULL) return -1L;

  og = &oggz->current_page;

#ifdef OGGZ_WRITE_DIRECT
  fd = fileno (oggz->file);
#endif

  h = MIN (n, og->header_len - writer->page_offset);
  if (h > 0) {
#ifdef OGGZ_WRITE_DIRECT
    nwritten = write (fd, og->header + writer->page_offset, h);
#else
    nwritten = (long)oggz_io_write (oggz, og->header + writer->page_offset, h);
#endif

#ifdef DEBUG
    if (nwritten < h) {
      printf ("oggz_page_writeout: %ld < %ld\n", nwritten, h);
    }
#endif
    writer->page_offset += h;
    n -= h;
  } else {
    h = 0;
  }
  
  b = MIN (n, og->header_len + og->body_len - writer->page_offset);
  if (b > 0) {
#ifdef OGGZ_WRITE_DIRECT
    nwritten = write (fd,
		      og->body + (writer->page_offset - og->header_len), b);
#else
    nwritten = (long)oggz_io_write (oggz, og->body + (writer->page_offset - og->header_len), b);
#endif
#ifdef DEBUG
    if (nwritten < b) {
      printf ("oggz_page_writeout: %ld < %ld\n", nwritten, b);
    }
#endif
    writer->page_offset += b;
    n -= b;
  } else {
    b = 0;
  }

  return h + b;
}

static oggz_writer_packet_t *
oggz_dequeue_packet (OGGZ * oggz)
{
  OggzWriter * writer = &oggz->x.writer;
  oggz_writer_packet_t * next_zpacket;

  if (writer->next_zpacket != NULL) {
    next_zpacket = writer->next_zpacket;
    writer->next_zpacket = NULL;
  } else {
    next_zpacket = oggz_vector_pop (writer->packet_queue);

    if (next_zpacket == NULL) {
      if (writer->hungry) {
	writer->hungry (oggz, 1, writer->hungry_user_data);
	next_zpacket = oggz_vector_pop (writer->packet_queue);
      }
    }
  }

  return next_zpacket;
}

static long
oggz_writer_make_packet (OGGZ * oggz)
{
  OggzWriter * writer = &oggz->x.writer;
  oggz_writer_packet_t * zpacket, * next_zpacket;
  int cb_ret = 0, ret = 0;

#ifdef DEBUG
  printf ("make packet ...\n");
#endif

  /* finished with current packet; unguard */
  zpacket = writer->current_zpacket;
  oggz_writer_packet_free (zpacket);
  writer->current_zpacket = NULL;

  /* if the user wants the hungry callback after every packet, give
   * it to them, marking emptiness appropriately
   */
  if (writer->hungry && !writer->hungry_only_when_empty) {
    int empty = (oggz_vector_size (writer->packet_queue) == 0);
    cb_ret = writer->hungry (oggz, empty, writer->hungry_user_data);
  }

  if (cb_ret == 0) {
    /* dequeue and init the next packet */
    if ((next_zpacket = oggz_dequeue_packet (oggz)) == NULL) {
      /*writer->eos = 1;*/
      ret = 0;
    } else {
      if ((writer->current_stream != NULL) &&
	  (next_zpacket->flush & OGGZ_FLUSH_BEFORE)) {
	writer->flushing = 1;
#ifdef DEBUG
	printf ("oggz_writer_make_packet: set flush to %d\n",
		writer->flushing);
#endif
	next_zpacket->flush &= OGGZ_FLUSH_AFTER;
	writer->next_zpacket = next_zpacket;
      } else {
	oggz_packet_init (oggz, next_zpacket);
      }
      ret = 1;
    }
  } else {
    ret = 0;
  }

  return ret;
}

long
oggz_write_output (OGGZ * oggz, unsigned char * buf, long n)
{
  OggzWriter * writer = &oggz->x.writer;
  long bytes, bytes_written = 1, remaining = n, nwritten = 0;
  int active = 1;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (!(oggz->flags & OGGZ_WRITE)) {
    return OGGZ_ERR_INVALID;
  }

  if (writer->writing) return OGGZ_ERR_RECURSIVE_WRITE;
  writer->writing = 1;

  while (active && remaining > 0) {
    bytes = MIN (remaining, 1024);

    while (writer->state == OGGZ_MAKING_PACKETS) {
      if (!oggz_writer_make_packet (oggz)) {
	active = 0;
	break;
      }
      if (oggz_page_init (oggz)) {
	writer->state = OGGZ_WRITING_PAGES;
      }
    }

    if (writer->state == OGGZ_WRITING_PAGES) {
      bytes_written = oggz_page_copyout (oggz, buf, bytes);

      if (bytes_written == -1) {
	active = 0;
	return OGGZ_ERR_SYSTEM; /* XXX: catch next */
      } else if (bytes_written == 0) {
	if (!oggz_page_init (oggz)) {
	  writer->state = OGGZ_MAKING_PACKETS;
	}
      }

      buf += bytes_written;

      remaining -= bytes_written;
      nwritten += bytes_written;
    }
  }

  writer->writing = 0;

  return nwritten;
}

long
oggz_write (OGGZ * oggz, long n)
{
  OggzWriter * writer = &oggz->x.writer;
  long bytes, bytes_written = 1, remaining = n, nwritten = 0;
  int active = 1;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (!(oggz->flags & OGGZ_WRITE)) {
    return OGGZ_ERR_INVALID;
  }

  if (writer->writing) return OGGZ_ERR_RECURSIVE_WRITE;
  writer->writing = 1;

#ifdef DEBUG
  printf ("oggz_write: IN\n");
#endif

  while (active && remaining > 0) {
    bytes = MIN (remaining, 1024);

#ifdef DEBUG
    printf ("oggz_write: write loop (%ld , %ld remain) ...\n", bytes,
	    remaining);
#endif

    while (writer->state == OGGZ_MAKING_PACKETS) {
      if (!oggz_writer_make_packet (oggz)) {
	active = 0;
#ifdef DEBUG
	printf ("oggz_write: no packets\n");
#endif
	break;
      }
      if (oggz_page_init (oggz)) {
	writer->state = OGGZ_WRITING_PAGES;
      }
    }

    if (writer->state == OGGZ_WRITING_PAGES) {
      bytes_written = oggz_page_writeout (oggz, bytes);

      if (bytes_written == -1) {
	active = 0;
	return OGGZ_ERR_SYSTEM; /* XXX: catch next */
      } else if (bytes_written == 0) {
	if (!oggz_page_init (oggz)) {
#ifdef DEBUG
	  printf ("oggz_write: bytes_written == 0, DONE\n");
#endif
	  writer->state = OGGZ_MAKING_PACKETS;
	}
      }

      remaining -= bytes_written;
      nwritten += bytes_written;      
    }
  }

#ifdef DEBUG
  printf ("oggz_write: OUT %ld\n", nwritten);
#endif

  writer->writing = 0;

  return nwritten;
}

long
oggz_write_get_next_page_size (OGGZ * oggz)
{
  OggzWriter * writer = &oggz->x.writer;
  ogg_page * og;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (!(oggz->flags & OGGZ_WRITE)) {
    return OGGZ_ERR_INVALID;
  }

  og = &oggz->current_page;

  return (og->header_len + og->body_len - (long)writer->page_offset);
}

#else /* OGGZ_CONFIG_WRITE */

#include <ogg/ogg.h>
#include "oggz_private.h"

int
oggz_write_set_hungry_callback (OGGZ * oggz, OggzWriteHungry hungry,
				int only_when_empty, void * user_data)
{
  return OGGZ_ERR_DISABLED;
}

int
oggz_write_feed (OGGZ * oggz, ogg_packet * op, long serialno, int flush,
		 int * guard)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_write_output (OGGZ * oggz, unsigned char * buf, long n)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_write (OGGZ * oggz, long n)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_write_get_next_page_size (OGGZ * oggz)
{
  return OGGZ_ERR_DISABLED;
}

#endif
