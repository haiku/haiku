/*
 * "$Id: sequence.h,v 1.2 2008/01/21 23:19:38 rlk Exp $"
 *
 *   libgimpprint sequence functions.
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
 * @file gutenprint/sequence.h
 * @brief Sequence functions.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gimpprint, etc.
 */

#ifndef GUTENPRINT_SEQUENCE_H
#define GUTENPRINT_SEQUENCE_H

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * The sequence is a simple "vector of numbers" data structure.
   *
   * @defgroup sequence sequence
   * @{
   */

struct stp_sequence;
  /** The sequence opaque data type. */
typedef struct stp_sequence stp_sequence_t;

  /**
   * Create a new sequence.
   * @returns the newly created sequence.
   */
extern stp_sequence_t *stp_sequence_create(void);

  /**
   * Destroy a sequence.
   * It is an error to destroy the sequence more than once.
   * @param sequence the sequence to destroy.
   */
extern void stp_sequence_destroy(stp_sequence_t *sequence);

  /**
   * Copy a sequence.
   * Both dest and source must be valid sequences previously created
   * with stp_sequence_create().
   * @param dest the destination sequence.
   * @param source the source sequence.
   */
extern void stp_sequence_copy(stp_sequence_t *dest,
			      const stp_sequence_t *source);

  /**
   * Copy and allocate a sequence.
   * A new sequence will be created, and then the contents of source will
   * be copied into it.  The destination must not have been previously
   * allocated with stp_sequence_create().
   * @param sequence the source sequence.
   * @returns the new copy of the sequence.
   */
extern stp_sequence_t *stp_sequence_create_copy(const stp_sequence_t *sequence);

  /**
   * Reverse a sequence.
   * Both dest and source must be valid sequences previously created
   * with stp_sequence_create().
   * @param dest the destination sequence.
   * @param source the source sequence.
   */
extern void stp_sequence_reverse(stp_sequence_t *dest,
				 const stp_sequence_t *source);

  /**
   * Reverse and allocate a sequence.
   * A new sequence will be created, and then the contents of source will
   * be copied into it.  The destination must not have been previously
   * allocated with stp_sequence_create().
   * @param sequence the source sequence.
   * @returns the new copy of the sequence.
   */
extern stp_sequence_t *stp_sequence_create_reverse(const stp_sequence_t *sequence);

  /**
   * Set the lower and upper bounds.
   * The lower and upper bounds set the minimum and maximum values
   * that a point in the sequence may hold.
   * @param sequence the sequence to work on.
   * @param low the lower bound.
   * @param high the upper bound.
   * @returns 1 on success, or 0 if the lower bound is greater than
   * the upper bound.
   */
extern int stp_sequence_set_bounds(stp_sequence_t *sequence,
				   double low, double high);

  /**
   * Get the lower and upper bounds.
   * The values are stored in the variables pointed to by low and
   * high.
   * @param sequence the sequence to work on.
   * @param low a pointer to a double to store the low bound in.
   * @param high a pointer to a double to store the high bound in.
   */
extern void stp_sequence_get_bounds(const stp_sequence_t *sequence,
				    double *low, double *high);


  /**
   * Get range of values stored in the sequence.
   * The values are stored in the variables pointed to by low and
   * high.
   * @param sequence the sequence to work on.
   * @param low a pointer to a double to store the low bound in.
   * @param high a pointer to a double to store the high bound in.
   */
extern void stp_sequence_get_range(const stp_sequence_t *sequence,
				   double *low, double *high);

  /**
   * Set the sequence size.
   * The size is the number of elements the sequence contains.  Note
   * that resizing will destroy all data contained in the sequence.
   * @param sequence the sequence to work on.
   * @param size the size to set the sequence to.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_size(stp_sequence_t *sequence, size_t size);

  /**
   * Get the sequence size.
   * @returns the sequence size.
   */
extern size_t stp_sequence_get_size(const stp_sequence_t *sequence);

  /**
   * Set the data in a sequence.
   * @param sequence the sequence to set.
   * @param count the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_data(stp_sequence_t *sequence,
				 size_t count,
				 const double *data);

  /**
   * Set the data in a subrange of a sequence.
   * @param sequence the sequence to set.
   * @param where the starting element in the sequence (indexed from
   * 0).
   * @param size the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_subrange(stp_sequence_t *sequence,
				     size_t where, size_t size,
				     const double *data);

  /**
   * Get the data in a sequence.
   * @param sequence the sequence to get the data from.
   * @param size the number of elements in the sequence are stored in
   * the size_t pointed to.
   * @param data a pointer to the first element of an sequence of doubles
   * is stored in a pointer to double*.
   * @code
   * stp_sequence_t *sequence;
   * size_t size;
   * double *data;
   * stp_sequence_get_data(sequence, &size, &data);
   * @endcode
   */
extern void stp_sequence_get_data(const stp_sequence_t *sequence,
				  size_t *size, const double **data);

  /**
   * Set the data at a single point in a sequence.
   * @param sequence the sequence to use.
   * @param where the location (indexed from zero).
   * @param data the datum to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_point(stp_sequence_t *sequence,
				  size_t where, double data);

  /**
   * Get the data at a single point in a sequence.
   * @param sequence the sequence to use.
   * @param where the location (indexed from zero).
   * @param data the datum is stored in the double pointed to.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_get_point(const stp_sequence_t *sequence,
				  size_t where, double *data);


  /**
   * Set the data in a sequence from float values.
   * @param sequence the sequence to set.
   * @param count the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_float_data(stp_sequence_t *sequence,
				       size_t count, const float *data);

  /**
   * Set the data in a sequence from long values.
   * @param sequence the sequence to set.
   * @param count the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_long_data(stp_sequence_t *sequence,
				      size_t count, const long *data);

  /**
   * Set the data in a sequence from unsigned long values.
   * @param sequence the sequence to set.
   * @param count the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_ulong_data(stp_sequence_t *sequence,
				       size_t count, const unsigned long *data);

  /**
   * Set the data in a sequence from int values.
   * @param sequence the sequence to set.
   * @param count the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_int_data(stp_sequence_t *sequence,
				     size_t count, const int *data);

  /**
   * Set the data in a sequence from unsigned int values.
   * @param sequence the sequence to set.
   * @param count the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_uint_data(stp_sequence_t *sequence,
				      size_t count, const unsigned int *data);

  /**
   * Set the data in a sequence from short values.
   * @param sequence the sequence to set.
   * @param count the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_short_data(stp_sequence_t *sequence,
				       size_t count, const short *data);

  /**
   * Set the data in a sequence from unsigned short values.
   * @param sequence the sequence to set.
   * @param count the number of elements in the data.
   * @param data a pointer to the first member of a sequence
   * containing the data to set.
   * @returns 1 on success, 0 on failure.
   */
extern int stp_sequence_set_ushort_data(stp_sequence_t *sequence,
					size_t count, const unsigned short *data);

  /**
   * Get the data in a sequence as float data.
   * The pointer returned is owned by the curve, and is not guaranteed
   * to be valid beyond the next non-const curve call;
   * If the bounds of the curve exceed the limits of the data type,
   * NULL is returned.
   * @param sequence the sequence to get the data from.
   * @param count the number of elements in the sequence are stored in
   * the size_t pointed to.
   * @returns a pointer to the first element of an sequence of floats
   * is stored in a pointer to float*.
   */
extern const float *stp_sequence_get_float_data(const stp_sequence_t *sequence,
						size_t *count);

  /**
   * Get the data in a sequence as long data.
   * The pointer returned is owned by the curve, and is not guaranteed
   * to be valid beyond the next non-const curve call;
   * If the bounds of the curve exceed the limits of the data type,
   * NULL is returned.
   * @param sequence the sequence to get the data from.
   * @param count the number of elements in the sequence are stored in
   * the size_t pointed to.
   * @returns a pointer to the first element of an sequence of longs
   * is stored in a pointer to long*.
   */
extern const long *stp_sequence_get_long_data(const stp_sequence_t *sequence,
					      size_t *count);

  /**
   * Get the data in a sequence as unsigned long data.
   * The pointer returned is owned by the curve, and is not guaranteed
   * to be valid beyond the next non-const curve call;
   * If the bounds of the curve exceed the limits of the data type,
   * NULL is returned.
   * @param sequence the sequence to get the data from.
   * @param count the number of elements in the sequence are stored in
   * the size_t pointed to.
   * @returns a pointer to the first element of an sequence of
   * unsigned longs is stored in a pointer to unsigned long*.
   */
extern const unsigned long *stp_sequence_get_ulong_data(const stp_sequence_t *sequence,
							size_t *count);

  /**
   * Get the data in a sequence as int data.
   * The pointer returned is owned by the curve, and is not guaranteed
   * to be valid beyond the next non-const curve call;
   * If the bounds of the curve exceed the limits of the data type,
   * NULL is returned.
   * @param sequence the sequence to get the data from.
   * @param count the number of elements in the sequence are stored in
   * the size_t pointed to.
   * @returns a pointer to the first element of an sequence of ints
   * is stored in a pointer to int*.
   */
extern const int *stp_sequence_get_int_data(const stp_sequence_t *sequence,
					    size_t *count);

  /**
   * Get the data in a sequence as unsigned int data.
   * The pointer returned is owned by the curve, and is not guaranteed
   * to be valid beyond the next non-const curve call;
   * If the bounds of the curve exceed the limits of the data type,
   * NULL is returned.
   * @param sequence the sequence to get the data from.
   * @param count the number of elements in the sequence are stored in
   * the size_t pointed to.
   * @returns a pointer to the first element of an sequence of
   * unsigned ints is stored in a pointer to unsigned int*.
   */
extern const unsigned int *stp_sequence_get_uint_data(const stp_sequence_t *sequence,
						      size_t *count);

  /**
   * Get the data in a sequence as short data.
   * The pointer returned is owned by the curve, and is not guaranteed
   * to be valid beyond the next non-const curve call;
   * If the bounds of the curve exceed the limits of the data type,
   * NULL is returned.
   * @param sequence the sequence to get the data from.
   * @param count the number of elements in the sequence are stored in
   * the size_t pointed to.
   * @returns a pointer to the first element of an sequence of shorts
   * is stored in a pointer to short*.
   */
extern const short *stp_sequence_get_short_data(const stp_sequence_t *sequence,
						size_t *count);

  /**
   * Get the data in a sequence as unsigned short data.
   * The pointer returned is owned by the curve, and is not guaranteed
   * to be valid beyond the next non-const curve call;
   * If the bounds of the curve exceed the limits of the data type,
   * NULL is returned.
   * @param sequence the sequence to get the data from.
   * @param count the number of elements in the sequence are stored in
   * the size_t pointed to.
   * @returns a pointer to the first element of an sequence of
   * unsigned shorts is stored in a pointer to unsigned short*.
   */
extern const unsigned short *stp_sequence_get_ushort_data(const stp_sequence_t *sequence,
							  size_t *count);

  /** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_SEQUENCE_H */
