/*
 * "$Id: generic-options.h,v 1.5 2004/09/17 18:38:20 rleigh Exp $"
 *
 *   Copyright 2003 Robert Krawitz (rlk@alum.mit.edu)
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

#ifndef GUTENPRINT_INTERNAL_GENERIC_OPTIONS_H
#define GUTENPRINT_INTERNAL_GENERIC_OPTIONS_H

typedef struct
{
  const char *name;
  const char *text;
  int quality_level;		/* Between 0 and 10 */
} stpi_quality_t;

typedef struct
{
  const char *name;
  const char *text;
} stpi_image_type_t;

typedef struct
{
  const char *name;
  const char *text;
} stpi_job_mode_t;

extern int stpi_get_qualities_count(void);

extern const stpi_quality_t *stpi_get_quality_by_index(int idx);

extern const stpi_quality_t *stpi_get_quality_by_name(const char *quality);

extern int stpi_get_image_types_count(void);

extern const stpi_image_type_t *stpi_get_image_type_by_index(int idx);

extern const stpi_image_type_t *stpi_get_image_type_by_name(const char *image_type);

extern int stpi_get_job_modes_count(void);

extern const stpi_job_mode_t *stpi_get_job_mode_by_index(int idx);

extern const stpi_job_mode_t *stpi_get_job_mode_by_name(const char *job_mode);

extern stp_parameter_list_t stp_list_generic_parameters(const stp_vars_t *v);

extern void stpi_describe_generic_parameter(const stp_vars_t *v,
					   const char *name,
					   stp_parameter_t *description);

#endif /* GUTENPRINT_INTERNAL_GENERIC_OPTIONS_H */
