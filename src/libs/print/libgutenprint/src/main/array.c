/*
 * "$Id: array.c,v 1.17 2010/08/04 00:33:55 rlk Exp $"
 *
 *   Array data type.  This type is designed to be derived from by
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
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>


struct stp_array
{
  stp_sequence_t *data; /* First member, to allow typecasting to sequence. */
  int x_size;
  int y_size;
};

/*
 * We could do more sanity checks here if we want.
 */
#define CHECK_ARRAY(array) STPI_ASSERT(array != NULL, NULL)

static void array_ctor(stp_array_t *array)
{
  array->data = stp_sequence_create();
  stp_sequence_set_size(array->data, array->x_size * array->y_size);
}

stp_array_t *
stp_array_create(int x_size, int y_size)
{
  stp_array_t *ret;
  ret = stp_zalloc(sizeof(stp_array_t));
  ret->x_size = x_size;
  ret->y_size = y_size;
  ret->data = NULL;
  array_ctor(ret);
  return ret;
}


static void
array_dtor(stp_array_t *array)
{
  if (array->data)
    stp_sequence_destroy(array->data);
  memset(array, 0, sizeof(stp_array_t));
}

void
stp_array_destroy(stp_array_t *array)
{
  CHECK_ARRAY(array);
  array_dtor(array);
  stp_free(array);
}

void
stp_array_copy(stp_array_t *dest, const stp_array_t *source)
{
  CHECK_ARRAY(dest);
  CHECK_ARRAY(source);

  dest->x_size = source->x_size;
  dest->y_size = source->y_size;
  if (dest->data)
    stp_sequence_destroy(dest->data);
  dest->data = stp_sequence_create_copy(source->data);
}

stp_array_t *
stp_array_create_copy(const stp_array_t *array)
{
  stp_array_t *ret;
  CHECK_ARRAY(array);
  ret = stp_array_create(0, 0); /* gets freed next */
  stp_array_copy(ret, array);
  return ret;
}


void
stp_array_set_size(stp_array_t *array, int x_size, int y_size)
{
  CHECK_ARRAY(array);
  if (array->data) /* Free old data */
    stp_sequence_destroy(array->data);
  array->x_size = x_size;
  array->y_size = y_size;
  array->data = stp_sequence_create();
  stp_sequence_set_size(array->data, array->x_size * array->y_size);
}

void
stp_array_get_size(const stp_array_t *array, int *x_size, int *y_size)
{
  CHECK_ARRAY(array);
  *x_size = array->x_size;
  *y_size = array->y_size;
  return;
}

void
stp_array_set_data(stp_array_t *array, const double *data)
{
  CHECK_ARRAY(array);
  stp_sequence_set_data(array->data, array->x_size * array->y_size,
			data);
}

void
stp_array_get_data(const stp_array_t *array, size_t *size, const double **data)
{
  CHECK_ARRAY(array);
  stp_sequence_get_data(array->data, size, data);
}

int
stp_array_set_point(stp_array_t *array, int x, int y, double data)
{
  CHECK_ARRAY(array);

  if (((array->x_size * x) + y) >= (array->x_size * array->y_size))
    return 0;

  return stp_sequence_set_point(array->data, (array->x_size * x) + y, data);}

int
stp_array_get_point(const stp_array_t *array, int x, int y, double *data)
{
  CHECK_ARRAY(array);

  if (((array->x_size * x) + y) >= array->x_size * array->y_size)
    return 0;
  return stp_sequence_get_point(array->data,
				(array->x_size * x) + y, data);
}

const stp_sequence_t *
stp_array_get_sequence(const stp_array_t *array)
{
  CHECK_ARRAY(array);

  return array->data;
}

stp_array_t *
stp_array_create_from_xmltree(stp_mxml_node_t *array)  /* The array node */
{
  const char *stmp;                          /* Temporary string */
  stp_mxml_node_t *child;                       /* Child sequence node */
  int x_size, y_size;
  size_t count;
  stp_sequence_t *seq = NULL;
  stp_array_t *ret = NULL;

  stmp = stp_mxmlElementGetAttr(array, "x-size");
  if (stmp)
    {
      x_size = (int) strtoul(stmp, NULL, 0);
    }
  else
    {
      stp_erprintf("stp_array_create_from_xmltree: \"x-size\" missing\n");
      goto error;
    }
  /* Get y-size */
  stmp = stp_mxmlElementGetAttr(array, "y-size");
  if (stmp)
    {
      y_size = (int) strtoul(stmp, NULL, 0);
    }
  else
    {
      stp_erprintf("stp_array_create_from_xmltree: \"y-size\" missing\n");
      goto error;
    }

  /* Get the sequence data */

  child = stp_mxmlFindElement(array, array, "sequence", NULL, NULL, STP_MXML_DESCEND);
  if (child)
    seq = stp_sequence_create_from_xmltree(child);

  if (seq == NULL)
    goto error;

  ret = stp_array_create(x_size, y_size);
  if (ret->data)
    stp_sequence_destroy(ret->data);
  ret->data = seq;

  count = stp_sequence_get_size(seq);
  if (count != (x_size * y_size))
    {
      stp_erprintf("stp_array_create_from_xmltree: size mismatch between array and sequence\n");
      goto error;
    }

  return ret;

 error:
  stp_erprintf("stp_array_create_from_xmltree: error during array read\n");
  if (ret)
    stp_array_destroy(ret);
  return NULL;
}

stp_mxml_node_t *
stp_xmltree_create_from_array(const stp_array_t *array)  /* The array */
{
  int x_size, y_size;
  char *xs, *ys;

  stp_mxml_node_t *arraynode = NULL;
  stp_mxml_node_t *child = NULL;

  stp_xml_init();

  /* Get array details */
  stp_array_get_size(array, &x_size, &y_size);

  /* Construct the allocated strings required */
  stp_asprintf(&xs, "%d", x_size);
  stp_asprintf(&ys, "%d", y_size);

  arraynode = stp_mxmlNewElement(NULL, "array");
  stp_mxmlElementSetAttr(arraynode, "x-size", xs);
  stp_mxmlElementSetAttr(arraynode, "y-size", ys);
  stp_free(xs);
  stp_free(ys);

  child = stp_xmltree_create_from_sequence(stp_array_get_sequence(array));

  if (child)
    stp_mxmlAdd(arraynode, STP_MXML_ADD_AFTER, NULL, child);
  else
    {
      stp_mxmlDelete(arraynode);
      arraynode = NULL;
    }

  stp_xml_exit();

  return arraynode;
}
