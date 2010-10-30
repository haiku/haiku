/*
 * "$Id: paper.h,v 1.4 2008/07/12 15:05:54 rlk Exp $"
 *
 *   libgimpprint paper functions.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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
 * @file gutenprint/paper.h
 * @brief Paper size functions.
 */

#ifndef GUTENPRINT_PAPER_H
#define GUTENPRINT_PAPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gutenprint/vars.h>

/**
 * The papersize describes the dimensions of a paper.
 *
 * @defgroup papersize papersize
 * @{
 */



/**
 * Units of measurement.
 */
typedef enum
{
  /** English/Imperial units. */
  PAPERSIZE_ENGLISH_STANDARD,
  /** Metric units. */
  PAPERSIZE_METRIC_STANDARD,
  /** English/Imperial units (optional paper, not displayed by default). */
  PAPERSIZE_ENGLISH_EXTENDED,
  /** Metric units (optional paper, not displayed by default). */
  PAPERSIZE_METRIC_EXTENDED
} stp_papersize_unit_t;

typedef enum
{
  /** Standard paper size */
  PAPERSIZE_TYPE_STANDARD = 0,
  /** Envelope */
  PAPERSIZE_TYPE_ENVELOPE
} stp_papersize_type_t;

/** The papersize data type. */
typedef struct
{
  /** Short unique name (not translated). */
  char *name;
  /** Long descriptive name (translated). */
  char *text;
  /** Comment. */
  char *comment;
  /** Paper width. */
  unsigned width;
  /** Paper height. */
  unsigned height;
  /** Top margin. */
  unsigned top;
  /** Left margin. */
  unsigned left;
  /** Bottom margin. */
  unsigned bottom;
  /** Right margin. */
  unsigned right;
  /** Units of measurement. */
  stp_papersize_unit_t paper_unit;
  /** Paper size type. */
  stp_papersize_type_t paper_size_type;
} stp_papersize_t;

/**
 * Get the number of available papersizes.
 * @returns the number of papersizes.
 */
extern int stp_known_papersizes(void);

/**
 * Get a papersize by name.
 * @param name the short unique name of the paper.
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stp_get_papersize_by_name(const char *name);

/**
 * Get a papersize by size.
 * The nearest available size to the size requested will be found.
 * Only paper sizes within 5 points of width and height will be considered.
 * @param length the length of the paper.
 * @param width the width of the paper
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stp_get_papersize_by_size(int length,
							int width);

/**
 * Get a papersize by size if an exact match is found.
 * @param length the length of the paper.
 * @param width the width of the paper
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stp_get_papersize_by_size_exact(int length,
							      int width);

/**
 * Get a papersize by its index number.
 * @param idx the index number.  This must not be greater than (total
 * number of papers - 1).
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stp_get_papersize_by_index(int idx);

/**
 * Get the default paper dimensions for the current configuration.
 * The default is derived from the PageSize parameter if set, otherwise
 * the default page size for the printer is used.  If no value can be
 * determined, 1x1 will be returned.
 * @param v the Gutenprint vars object
 * @param width pointer to storage that the width will be returned in.
 * @param height pointer to storage that the height will be returned in.
 */
extern void stp_default_media_size(const stp_vars_t *v,
				   int *width, int *height);

/** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_PAPER_H */
/*
 * End of "$Id: paper.h,v 1.4 2008/07/12 15:05:54 rlk Exp $".
 */
