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

#include "oggz_compat.h"
#include "oggz_private.h"
#include "oggz_vector.h"

/*#define DEBUG*/

static int
oggz_flags_disabled (int flags)
{
 if (flags & OGGZ_WRITE) {
   if (!OGGZ_CONFIG_WRITE) return OGGZ_ERR_DISABLED;
 } else {
   if (!OGGZ_CONFIG_READ) return OGGZ_ERR_DISABLED;
 }

 return 0;
}

OGGZ *
oggz_new (int flags)
{
  OGGZ * oggz;

  if (oggz_flags_disabled (flags)) return NULL;

  oggz = (OGGZ *) oggz_malloc (sizeof (OGGZ));
  if (oggz == NULL) return NULL;

  oggz->flags = flags;
  oggz->file = NULL;
  oggz->io = NULL;

  oggz->offset = 0;
  oggz->offset_data_begin = 0;

  oggz->streams = oggz_vector_new ();
  oggz->all_at_eos = 0;

  oggz->metric = NULL;
  oggz->metric_user_data = NULL;
  oggz->metric_internal = 0;

  oggz->order = NULL;
  oggz->order_user_data = NULL;

  if (OGGZ_CONFIG_WRITE && (oggz->flags & OGGZ_WRITE)) {
    oggz_write_init (oggz);
  } else if (OGGZ_CONFIG_READ) {
    oggz_read_init (oggz);
  }

  return oggz;
}

OGGZ *
oggz_open (char * filename, int flags)
{
  OGGZ * oggz = NULL;
  FILE * file = NULL;

  if (oggz_flags_disabled (flags)) return NULL;

  if (flags & OGGZ_WRITE) {
    file = fopen (filename, "wb");
  } else {
    file = fopen (filename, "rb");
  }
  if (file == NULL) return NULL;

  oggz = oggz_new (flags);
  oggz->file = file;

  return oggz;
}

OGGZ *
oggz_open_stdio (FILE * file, int flags)
{
  OGGZ * oggz = NULL;

  if (oggz_flags_disabled (flags)) return NULL;

  oggz = oggz_new (flags);
  oggz->file = file;

  return oggz;
}

int
oggz_flush (OGGZ * oggz)
{
  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  return oggz_io_flush (oggz);
}

static int
oggz_stream_clear (void * data)
{
  oggz_stream_t * stream = (oggz_stream_t *) data;

  if (stream->ogg_stream.serialno != -1)
    ogg_stream_clear (&stream->ogg_stream);

  if (stream->metric_internal)
    oggz_free (stream->metric_user_data);

  return 0;
}

int
oggz_close (OGGZ * oggz)
{
  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  oggz_vector_foreach (oggz->streams, oggz_stream_clear);
  oggz_vector_delete (oggz->streams);

  if (OGGZ_CONFIG_WRITE && (oggz->flags & OGGZ_WRITE)) {
    oggz_write_close (oggz);
  } else if (OGGZ_CONFIG_READ) {
    oggz_read_close (oggz);
  }

  if (oggz->file != NULL) {
    if (fclose (oggz->file) == EOF) {
      return OGGZ_ERR_SYSTEM;
    }
  }

  oggz_free (oggz);

  return 0;
}

off_t
oggz_tell (OGGZ * oggz)
{
  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  return oggz->offset;
}

ogg_int64_t
oggz_tell_units (OGGZ * oggz)
{
  OggzReader * reader;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (oggz->flags & OGGZ_WRITE) {
    return OGGZ_ERR_INVALID;
  }

  reader = &oggz->x.reader;

  if (OGGZ_CONFIG_READ) {
    return reader->current_unit;
  } else {
    return OGGZ_ERR_DISABLED;
  }
}

/******** oggz_stream management ********/

static int
oggz_find_stream (void * data, long serialno)
{
  oggz_stream_t * stream = (oggz_stream_t *) data;

  return (stream->ogg_stream.serialno == serialno);
}

oggz_stream_t *
oggz_get_stream (OGGZ * oggz, long serialno)
{
  if (serialno < 0) return NULL;

  return oggz_vector_find (oggz->streams, oggz_find_stream, serialno);
}

oggz_stream_t *
oggz_add_stream (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;

  stream = oggz_malloc (sizeof (oggz_stream_t));
  if (stream == NULL) return NULL;

  ogg_stream_init (&stream->ogg_stream, (int)serialno);

  stream->delivered_non_b_o_s = 0;
  stream->b_o_s = 1;
  stream->e_o_s = 0;
  stream->granulepos = 0;
  stream->packetno = -1; /* will be incremented on first write */

  stream->metric = NULL;
  stream->metric_user_data = NULL;
  stream->metric_internal = 0;
  stream->order = NULL;
  stream->order_user_data = NULL;
  stream->read_packet = NULL;
  stream->read_user_data = NULL;

  oggz_vector_insert_p (oggz->streams, stream);

  return stream;
}

int
oggz_get_bos (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;
  int i, size;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (serialno == -1) {
    size = oggz_vector_size (oggz->streams);
    for (i = 0; i < size; i++) {
      stream = (oggz_stream_t *)oggz_vector_nth_p (oggz->streams, i);
#if 1
      /* If this stream has delivered a non bos packet, return FALSE */
      if (stream->delivered_non_b_o_s) return 0;
#else
      /* If this stream has delivered its bos packet, return FALSE */
      if (!stream->b_o_s) return 0;
#endif
    }
    return 1;
  } else {
    stream = oggz_get_stream (oggz, serialno);
    if (stream == NULL)
      return OGGZ_ERR_BAD_SERIALNO;

    return stream->b_o_s;
  }
}

int
oggz_get_eos (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;
  int i, size;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (serialno == -1) {
    size = oggz_vector_size (oggz->streams);
    for (i = 0; i < size; i++) {
      stream = (oggz_stream_t *)oggz_vector_nth_p (oggz->streams, i);
      if (!stream->e_o_s) return 0;
    }
    return 1;
  } else {
    stream = oggz_get_stream (oggz, serialno);
    if (stream == NULL)
      return OGGZ_ERR_BAD_SERIALNO;

    return stream->e_o_s;
  }
}

int
oggz_set_eos (OGGZ * oggz, long serialno)
{
  oggz_stream_t * stream;
  int i, size;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (serialno == -1) {
    size = oggz_vector_size (oggz->streams);
    for (i = 0; i < size; i++) {
      stream = (oggz_stream_t *) oggz_vector_nth_p (oggz->streams, i);
      stream->e_o_s = 1;
    }
    oggz->all_at_eos = 1;
  } else {
    stream = oggz_get_stream (oggz, serialno);
    if (stream == NULL)
      return OGGZ_ERR_BAD_SERIALNO;

    stream->e_o_s = 1;

    if (oggz_get_eos (oggz, -1))
      oggz->all_at_eos = 1;
  }

  return 0;
}

long
oggz_serialno_new (OGGZ * oggz)
{
  long serialno;

  do {
    serialno = oggz_random();
  } while (oggz_get_stream (oggz, serialno) != NULL);

  return serialno;
}

/******** OggzMetric management ********/

typedef struct {
  ogg_int64_t gr_n;
  ogg_int64_t gr_d;
} oggz_metric_linear_t;

static ogg_int64_t
oggz_metric_default_linear (OGGZ * oggz, long serialno, ogg_int64_t granulepos,
			    void * user_data)
{
  oggz_metric_linear_t * ldata = (oggz_metric_linear_t *)user_data;

  return (ldata->gr_d * granulepos / ldata->gr_n);
}

int
oggz_set_metric_internal (OGGZ * oggz, long serialno,
			  OggzMetric metric, void * user_data, int internal)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (serialno == -1) {
    if (oggz->metric_internal)
      oggz_free (oggz->metric_user_data);
    oggz->metric = metric;
    oggz->metric_user_data = user_data;
    oggz->metric_internal = internal;
  } else {
    stream = oggz_get_stream (oggz, serialno);
    if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

    if (stream->metric_internal)
      oggz_free (stream->metric_user_data);
    stream->metric = metric;
    stream->metric_user_data = user_data;
    stream->metric_internal = internal;
  }

  return 0;
}

int
oggz_set_metric (OGGZ * oggz, long serialno,
		 OggzMetric metric, void * user_data)
{
  return oggz_set_metric_internal (oggz, serialno, metric, user_data, 0);
}

int
oggz_set_metric_linear (OGGZ * oggz, long serialno,
			ogg_int64_t granule_rate_numerator,
			ogg_int64_t granule_rate_denominator)
{
  oggz_metric_linear_t * linear_data;

  /* we divide by the granulerate, ie. mult by gr_d/gr_n, so ensure
   * numerator is non-zero */
  if (granule_rate_numerator == 0) {
    granule_rate_numerator = 1;
    granule_rate_denominator = 0;
  }

  linear_data = oggz_malloc (sizeof (oggz_metric_linear_t));
  linear_data->gr_n = granule_rate_numerator;
  linear_data->gr_d = granule_rate_denominator;

  return oggz_set_metric_internal (oggz, serialno, oggz_metric_default_linear,
				   linear_data, 1);
}

/*
 * Check if an oggz has metrics for all streams
 */
int
oggz_has_metrics (OGGZ * oggz)
{
  int i, size;
  oggz_stream_t * stream;

  if (oggz->metric != NULL) return 1;

  size = oggz_vector_size (oggz->streams);
  for (i = 0; i < size; i++) {
    stream = (oggz_stream_t *)oggz_vector_nth_p (oggz->streams, i);
    if (stream->metric == NULL) return 0;
  }

  return 1;
}

ogg_int64_t
oggz_get_unit (OGGZ * oggz, long serialno, ogg_int64_t granulepos)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (granulepos == -1) return -1;

  if (serialno == -1) {
    if (oggz->metric)
      return oggz->metric (oggz, serialno, granulepos,
			   oggz->metric_user_data);
  } else {
    stream = oggz_get_stream (oggz, serialno);
    if (!stream) return -1;

    if (stream->metric) {
      return stream->metric (oggz, serialno, granulepos,
			     stream->metric_user_data);
    } else if (oggz->metric) {
      return oggz->metric (oggz, serialno, granulepos,
			   oggz->metric_user_data);
    }
  }

  return -1;
}

int
oggz_set_order (OGGZ * oggz, long serialno,
		OggzOrder order, void * user_data)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  if (oggz->flags & OGGZ_WRITE) {
    return OGGZ_ERR_INVALID;
  }

  if (serialno == -1) {
    oggz->order = order;
    oggz->order_user_data = user_data;
  } else {
    stream = oggz_get_stream (oggz, serialno);
    stream->order = order;
    stream->order_user_data = user_data;
  }

  return 0;
}

