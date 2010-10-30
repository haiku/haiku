/*
 * "$Id: array.h,v 1.1 2004/09/17 18:38:01 rleigh Exp $"
 *
 *   Copyright 2003 Roger Leigh (rleigh@debian.org)
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

/**
 * @file gutenprint/array.h
 * @brief Array functions.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gimpprint, etc.
 */

#ifndef GUTENPRINT_ARRAY_H
#define GUTENPRINT_ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gutenprint/sequence.h>


  /**
   * The array is a simple "two-dimensional array of numbers" data
   * structure.  array "inherits" from the sequence data structure
   * (implemented via containment).
   *
   * @defgroup array array
   * @{
   */

struct stp_array;
  /** The array opaque data type. */
typedef struct stp_array stp_array_t;

  /**
   * Create a new array.
   * The total size of the array will be (x_size * y_size).
   * @param x_size the number of "columns".
   * @param y_size the number of "rows".
   * @returns the newly created array.
   */
extern stp_array_t *stp_array_create(int x_size, int y_size);

  /**
   * Destroy an array.
   * It is an error to destroy the array more than once.
   * @param array the array to destroy.
   */
extern void stp_array_destroy(stp_array_t *array);

  /**
   * Copy an array.
   * Both dest and source must be valid arrays previously created with
   * stp_array_create().
   * @param dest the destination array.
   * @param source the source array.
   */
extern void stp_array_copy(stp_array_t *dest, const stp_array_t *source);

  /**
   * Copy and allocate an array.
   * dest will be created, and then the contents of source will be
   * copied into it.  dest must not have been previously allocated
   * with stp_array_create().
   * @param array the source array.
   * @returns the new copy of the array.
   */
extern stp_array_t *stp_array_create_copy(const stp_array_t *array);

  /**
   * Resize an array.
   * Resizing an array will destroy all data stored in the array.
   * @param array the array to resize.
   * @param x_size the new number of "columns".
   * @param y_size the new number of "rows".
   */
extern void stp_array_set_size(stp_array_t *array, int x_size, int y_size);

  /**
   * Get the size of an array.
   * The current x and y sizes are stored in the integers pointed to
   * by x_size and y_size.
   * @param array the array to get the size of.
   * @param x_size a pointer to an integer to store the x size in.
   * @param y_size a pointer to an integer to store the y size in.
   */
extern void stp_array_get_size(const stp_array_t *array, int *x_size, int *y_size);

  /**
   * Set the data in an array.
   * @param array the array to set.
   * @param data a pointer to the first member of an array containing
   * the data to set.  This array must be at least as long as (x_size
   * * y_size).
   */
extern void stp_array_set_data(stp_array_t *array, const double *data);

  /**
   * Get the data in an array.
   * @param array the array to get the data from.
   * @param size the number of elements in the array (x_size * y_size)
   * are stored in the size_t pointed to.
   * @param data a pointer to the first element of an array of doubles
   * is stored in a pointer to double*.
   * @code
   * stp_array_t *array;
   * size_t size;
   * double *data;
   * stp_array_get_data(array, &size, &data);
   * @endcode
   */
extern void stp_array_get_data(const stp_array_t *array, size_t *size,
			       const double **data);

  /**
   * Set the data at a single point in the array.
   * @param array the array to use.
   * @param x the x location.
   * @param y the y location.
   * @param data the datum to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_array_set_point(stp_array_t *array, int x, int y,
			       double data);

  /**
   * Get the data at a single point in the array.
   * @param array the array to use.
   * @param x the x location.
   * @param y the y location.
   * @param data the datum is stored in the double pointed to.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_array_get_point(const stp_array_t *array, int x, int y,
			       double *data);

  /**
   * Get the underlying stp_sequence_t.
   * @param array the array to use.
   * @returns the (constant) stp_sequence_t.
   */
extern const stp_sequence_t *stp_array_get_sequence(const stp_array_t *array);

  /** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_ARRAY_H */
