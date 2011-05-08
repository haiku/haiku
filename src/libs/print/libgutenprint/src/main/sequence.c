/*
 * "$Id: sequence.c,v 1.29 2011/02/17 02:15:18 rlk Exp $"
 *
 *   Sequence data type.  This type is designed to be derived from by
 *   the curve and dither matrix types.
 *
 *   Copyright 2002-2003 Robert Krawitz (rlk@alum.mit.edu)
 *   Copyright 2003      Roger Leigh (rleigh@debian.org)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <gutenprint/sequence.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <math.h>
#ifdef sun
#include <ieeefp.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

struct stp_sequence
{
  int recompute_range; /* Do we need to recompute the min and max? */
  double blo;          /* Lower bound */
  double bhi;          /* Upper bound */
  double rlo;          /* Lower range limit */
  double rhi;          /* Upper range limit */
  size_t size;         /* Number of points */
  double *data;        /* Array of doubles */
  float *float_data;   /* Data converted to other form */
  long *long_data;
  unsigned long *ulong_data;
  int *int_data;
  unsigned *uint_data;
  short *short_data;
  unsigned short *ushort_data;
};

/*
 * We could do more sanity checks here if we want.
 */

#define CHECK_SEQUENCE(sequence) STPI_ASSERT(sequence, NULL)

static inline stp_sequence_t *
deconst_sequence(const stp_sequence_t *sequence)
{
  return (stp_sequence_t *) sequence;
}

static void
sequence_ctor(stp_sequence_t *sequence)
{
  sequence->rlo = sequence->blo = 0.0;
  sequence->rhi = sequence->bhi = 1.0;
  sequence->recompute_range = 1;
  sequence->size = 0;
  sequence->data = NULL;
}

stp_sequence_t *
stp_sequence_create(void)
{
  stp_sequence_t *ret;
  ret = stp_zalloc(sizeof(stp_sequence_t));
  sequence_ctor(ret);
  return ret;
}

static void
invalidate_auxilliary_data(stp_sequence_t *sequence)
{
  STP_SAFE_FREE(sequence->float_data);
  STP_SAFE_FREE(sequence->long_data);
  STP_SAFE_FREE(sequence->ulong_data);
  STP_SAFE_FREE(sequence->int_data);
  STP_SAFE_FREE(sequence->uint_data);
  STP_SAFE_FREE(sequence->short_data);
  STP_SAFE_FREE(sequence->ushort_data);
}

static void
sequence_dtor(stp_sequence_t *sequence)
{
  invalidate_auxilliary_data(sequence);
  if (sequence->data)
    stp_free(sequence->data);
  memset(sequence, 0, sizeof(stp_sequence_t));
}

void
stp_sequence_destroy(stp_sequence_t *sequence)
{
  CHECK_SEQUENCE(sequence);
  sequence_dtor(sequence);
  stp_free(sequence);
}

void
stp_sequence_copy(stp_sequence_t *dest, const stp_sequence_t *source)
{
  CHECK_SEQUENCE(dest);
  CHECK_SEQUENCE(source);

  dest->recompute_range = source->recompute_range;
  dest->blo = source->blo;
  dest->bhi = source->bhi;
  dest->rlo = source->rlo;
  dest->rhi = source->rhi;
  dest->size = source->size;
  dest->data = stp_zalloc(sizeof(double) * source->size);
  memcpy(dest->data, source->data, (sizeof(double) * source->size));
}

void
stp_sequence_reverse(stp_sequence_t *dest, const stp_sequence_t *source)
{
  int i;
  CHECK_SEQUENCE(dest);
  CHECK_SEQUENCE(source);

  dest->recompute_range = source->recompute_range;
  dest->blo = source->blo;
  dest->bhi = source->bhi;
  dest->rlo = source->rlo;
  dest->rhi = source->rhi;
  dest->size = source->size;
  dest->data = stp_zalloc(sizeof(double) * source->size);
  for (i = 0; i < source->size; i++)
    dest->data[i] = source->data[source->size - i - 1];
}

stp_sequence_t *
stp_sequence_create_copy(const stp_sequence_t *sequence)
{
  stp_sequence_t *ret;
  CHECK_SEQUENCE(sequence);
  ret = stp_sequence_create();
  stp_sequence_copy(ret, sequence);
  return ret;
}

stp_sequence_t *
stp_sequence_create_reverse(const stp_sequence_t *sequence)
{
  stp_sequence_t *ret;
  CHECK_SEQUENCE(sequence);
  ret = stp_sequence_create();
  stp_sequence_reverse(ret, sequence);
  return ret;
}

int
stp_sequence_set_bounds(stp_sequence_t *sequence, double low, double high)
{
  CHECK_SEQUENCE(sequence);
  if (low > high)
    return 0;
  sequence->rlo = sequence->blo = low;
  sequence->rhi = sequence->bhi = high;
  sequence->recompute_range = 1;
  return 1;
}

void
stp_sequence_get_bounds(const stp_sequence_t *sequence,
			double *low, double *high)
{
  CHECK_SEQUENCE(sequence);
  *low = sequence->blo;
  *high = sequence->bhi;
}


/*
 * Find the minimum and maximum points on the curve.
 */
static void
scan_sequence_range(stp_sequence_t *sequence)
{
  int i;
  sequence->rlo = sequence->bhi;
  sequence->rhi = sequence->blo;
  if (sequence->size)
    for (i = 0; i < sequence->size; i++)
      {
	if (sequence->data[i] < sequence->rlo)
	  sequence->rlo = sequence->data[i];
	if (sequence->data[i] > sequence->rhi)
	  sequence->rhi = sequence->data[i];
      }
  sequence->recompute_range = 0; /* Don't recompute unless the data changes */
}

void
stp_sequence_get_range(const stp_sequence_t *sequence,
		       double *low, double *high)
{
  if (sequence->recompute_range) /* Don't recompute the range if we don't
			       need to. */
    scan_sequence_range(deconst_sequence(sequence));
  *low = sequence->rlo;
  *high = sequence->rhi;
}


int
stp_sequence_set_size(stp_sequence_t *sequence, size_t size)
{
  CHECK_SEQUENCE(sequence);
  if (sequence->data) /* Free old data */
    {
      stp_free(sequence->data);
      sequence->data = NULL;
    }
  sequence->size = size;
  sequence->recompute_range = 1; /* Always recompute on change */
  if (size == 0)
    return 1;
  invalidate_auxilliary_data(sequence);
  sequence->data = stp_zalloc(sizeof(double) * size);
  return 1;
}


size_t
stp_sequence_get_size(const stp_sequence_t *sequence)
{
  CHECK_SEQUENCE(sequence);
  return sequence->size;
}



int
stp_sequence_set_data(stp_sequence_t *sequence,
		      size_t size, const double *data)
{
  CHECK_SEQUENCE(sequence);
  sequence->size = size;
  if (sequence->data)
    stp_free(sequence->data);
  sequence->data = stp_zalloc(sizeof(double) * size);
  memcpy(sequence->data, data, (sizeof(double) * size));
  invalidate_auxilliary_data(sequence);
  sequence->recompute_range = 1;
  return 1;
}

int
stp_sequence_set_subrange(stp_sequence_t *sequence, size_t where,
			  size_t size, const double *data)
{
  CHECK_SEQUENCE(sequence);
  if (where + size > sequence->size) /* Exceeds data size */
    return 0;
  memcpy(sequence->data+where, data, (sizeof(double) * size));
  invalidate_auxilliary_data(sequence);
  sequence->recompute_range = 1;
  return 1;
}


void
stp_sequence_get_data(const stp_sequence_t *sequence, size_t *size,
		      const double **data)
{
  CHECK_SEQUENCE(sequence);
  *size = sequence->size;
  *data = sequence->data;
}


int
stp_sequence_set_point(stp_sequence_t *sequence, size_t where,
		       double data)
{
  CHECK_SEQUENCE(sequence);

  if (where >= sequence->size || ! isfinite(data) ||
      data < sequence->blo || data > sequence->bhi)
    return 0;

  if (sequence->recompute_range == 0 && (data < sequence->rlo ||
					 data > sequence->rhi ||
					 sequence->data[where] == sequence->rhi ||
					 sequence->data[where] == sequence->rlo))
    sequence->recompute_range = 1;

  sequence->data[where] = data;
  invalidate_auxilliary_data(sequence);
  return 1;
}

int
stp_sequence_get_point(const stp_sequence_t *sequence, size_t where,
		       double *data)
{
  CHECK_SEQUENCE(sequence);

  if (where >= sequence->size)
    return 0;
  *data = sequence->data[where];
  return 1;
}

stp_sequence_t *
stp_sequence_create_from_xmltree(stp_mxml_node_t *da)
{
  const char *stmp;
  stp_sequence_t *ret = NULL;
  size_t point_count;
  double low, high;
  int i;

  ret = stp_sequence_create();

  /* Get curve point count */
  stmp = stp_mxmlElementGetAttr(da, "count");
  if (stmp)
    {
      point_count = (size_t) stp_xmlstrtoul(stmp);
      if ((stp_xmlstrtol(stmp)) < 0)
	{
	  stp_erprintf("stp_sequence_create_from_xmltree: \"count\" is less than zero\n");
	  goto error;
	}
    }
  else
    {
      stp_erprintf("stp_sequence_create_from_xmltree: \"count\" missing\n");
      goto error;
    }
  /* Get lower bound */
  stmp = stp_mxmlElementGetAttr(da, "lower-bound");
  if (stmp)
    {
      low = stp_xmlstrtod(stmp);
    }
  else
    {
      stp_erprintf("stp_sequence_create_from_xmltree: \"lower-bound\" missing\n");
      goto error;
    }
  /* Get upper bound */
  stmp = stp_mxmlElementGetAttr(da, "upper-bound");
  if (stmp)
    {
      high = stp_xmlstrtod(stmp);
    }
  else
    {
      stp_erprintf("stp_sequence_create_from_xmltree: \"upper-bound\" missing\n");
      goto error;
    }

  stp_deprintf(STP_DBG_XML,
	       "stp_sequence_create_from_xmltree: stp_sequence_set_size: %ld\n",
	       (long)point_count);
  stp_sequence_set_size(ret, point_count);
  stp_sequence_set_bounds(ret, low, high);

  /* Now read in the data points */
  if (point_count)
    {
      stp_mxml_node_t *child = da->child;
      i = 0;
      while (child && i < point_count)
	{
	  if (child->type == STP_MXML_TEXT)
	    {
	      char *endptr;
	      double tmpval = strtod(child->value.text.string, &endptr);
	      if (endptr == child->value.text.string)
		{
		  stp_erprintf
		    ("stp_sequence_create_from_xmltree: bad data %s\n",
		     child->value.text.string);
		  goto error;
		}
	      if (! isfinite(tmpval)
		  || ( tmpval == 0 && errno == ERANGE )
		  || tmpval < low
		  || tmpval > high)
		{
		  stp_erprintf("stp_sequence_create_from_xmltree: "
			       "read aborted: datum out of bounds: "
			       "%g (require %g <= x <= %g), n = %d\n",
			       tmpval, low, high, i);
		  goto error;
		}
	      /* Datum was valid, so now add to the sequence */
	      stp_sequence_set_point(ret, i, tmpval);
	      i++;
	    }
	  child = child->next;
	}
      if (i < point_count)
	{
	  stp_erprintf("stp_sequence_create_from_xmltree: "
		       "read aborted: too little data "
		       "(n=%d, needed %ld)\n", i, (long)point_count);
	  goto error;
	}
    }

  return ret;

 error:
  stp_erprintf("stp_sequence_create_from_xmltree: error during sequence read\n");
  if (ret)
    stp_sequence_destroy(ret);
  return NULL;
}

stp_mxml_node_t *
stp_xmltree_create_from_sequence(const stp_sequence_t *seq)   /* The sequence */
{
  size_t pointcount;
  double low;
  double high;

  char *count;
  char *lower_bound;
  char *upper_bound;

  stp_mxml_node_t *seqnode;

  int i;                 /* loop counter */

  pointcount = stp_sequence_get_size(seq);
  stp_sequence_get_bounds(seq, &low, &high);

  /* should count be of greater precision? */
  stp_asprintf(&count, "%lu", (unsigned long) pointcount);
  stp_asprintf(&lower_bound, "%g", low);
  stp_asprintf(&upper_bound, "%g", high);

  seqnode = stp_mxmlNewElement(NULL, "sequence");
  (void) stp_mxmlElementSetAttr(seqnode, "count", count);
  (void) stp_mxmlElementSetAttr(seqnode, "lower-bound", lower_bound);
  (void) stp_mxmlElementSetAttr(seqnode, "upper-bound", upper_bound);

  stp_free(count);
  stp_free(lower_bound);
  stp_free(upper_bound);

  /* Write the curve points into the node content */
  if (pointcount) /* Is there any data to write? */
    {
      for (i = 0; i < pointcount; i++)
	{
	  double dval;
	  char *sval;

	  if ((stp_sequence_get_point(seq, i, &dval)) != 1)
	    goto error;

	  stp_asprintf(&sval, "%g", dval);
	  stp_mxmlNewText(seqnode, 1, sval);
	  stp_free(sval);
      }
    }
  return seqnode;

 error:
  if (seqnode)
    stp_mxmlDelete(seqnode);
  return NULL;
}


#define isfinite_null(x) (1)

/* "Overloaded" functions */

#define DEFINE_DATA_SETTER(t, name, checkfinite)		\
int								\
stp_sequence_set_##name##_data(stp_sequence_t *sequence,	\
                               size_t count, const t *data)	\
{								\
  int i;							\
  CHECK_SEQUENCE(sequence);					\
  if (count < 2)						\
    return 0;							\
								\
  /* Validate the data before we commit to it. */		\
  for (i = 0; i < count; i++)					\
    if ((! checkfinite(data[i])) ||				\
	data[i] < sequence->blo ||				\
        data[i] > sequence->bhi)				\
      return 0;							\
  stp_sequence_set_size(sequence, count);			\
  for (i = 0; i < count; i++)					\
    stp_sequence_set_point(sequence, i, (double) data[i]);	\
  return 1;							\
}

DEFINE_DATA_SETTER(float, float, isfinite)
DEFINE_DATA_SETTER(long, long, isfinite_null)
DEFINE_DATA_SETTER(unsigned long, ulong, isfinite_null)
DEFINE_DATA_SETTER(int, int, isfinite_null)
DEFINE_DATA_SETTER(unsigned int, uint, isfinite_null)
DEFINE_DATA_SETTER(short, short, isfinite_null)
DEFINE_DATA_SETTER(unsigned short, ushort, isfinite_null)

#define DEFINE_DATA_ACCESSOR(t, lb, ub, name)				      \
const t *								      \
stp_sequence_get_##name##_data(const stp_sequence_t *sequence, size_t *count) \
{									      \
  int i;								      \
  CHECK_SEQUENCE(sequence);						      \
  if (sequence->blo < (double) lb || sequence->bhi > (double) ub)	      \
    return NULL;							      \
  if (!sequence->name##_data)						      \
    {									      \
      stp_sequence_t *seq = deconst_sequence(sequence);			      \
      seq->name##_data = stp_zalloc(sizeof(t) * sequence->size);	      \
      for (i = 0; i < sequence->size; i++)				      \
	seq->name##_data[i] = (t) sequence->data[i];			      \
    }									      \
  *count = sequence->size;						      \
  return sequence->name##_data;						      \
}

#ifndef HUGE_VALF /* ISO constant, from <math.h> */
#define HUGE_VALF 3.402823466E+38F
#endif

DEFINE_DATA_ACCESSOR(float, -HUGE_VALF, HUGE_VALF, float)
DEFINE_DATA_ACCESSOR(long, LONG_MIN, LONG_MAX, long)
DEFINE_DATA_ACCESSOR(unsigned long, 0, ULONG_MAX, ulong)
DEFINE_DATA_ACCESSOR(int, INT_MIN, INT_MAX, int)
DEFINE_DATA_ACCESSOR(unsigned int, 0, UINT_MAX, uint)
DEFINE_DATA_ACCESSOR(short, SHRT_MIN, SHRT_MAX, short)
DEFINE_DATA_ACCESSOR(unsigned short, 0, USHRT_MAX, ushort)
