/*
 * "$Id: string-list.h,v 1.3 2006/07/07 22:49:14 rleigh Exp $"
 *
 *   libgimpprint string list functions.
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
 * @file gutenprint/string-list.h
 * @brief String list functions.
 */

#ifndef GUTENPRINT_STRING_LIST_H
#define GUTENPRINT_STRING_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

struct stp_string_list;
/** The string_list opaque data type. */
typedef struct stp_string_list stp_string_list_t;

/**
 * String parameter.
 * Representation of a choice list of strings.  The choices themselves
 * consist of a key and a human-readable name.  The list object is
 * opaque.
 */
typedef struct
{
  const char	*name,	/*!< Option name (key, untranslated). */
		*text;	/*!< Human-readable (translated) text. */
} stp_param_string_t;

/****************************************************************
*                                                               *
* LISTS OF STRINGS                                              *
*                                                               *
****************************************************************/

/* The string_list opaque data type is defined in vars.h */

extern stp_string_list_t *
stp_string_list_create(void);

extern void
stp_string_list_destroy(stp_string_list_t *list);

extern stp_param_string_t *
stp_string_list_param(const stp_string_list_t *list, size_t element);

extern stp_param_string_t *
stp_string_list_find(const stp_string_list_t *list, const char *name);

extern size_t
stp_string_list_count(const stp_string_list_t *list);

extern stp_string_list_t *
stp_string_list_create_copy(const stp_string_list_t *list);

extern void
stp_string_list_add_string(stp_string_list_t *list,
			   const char *name, const char *text);

extern void
stp_string_list_remove_string(stp_string_list_t *list, const char *name);

extern stp_string_list_t *
stp_string_list_create_from_params(const stp_param_string_t *list,
				   size_t count);

extern int
stp_string_list_is_present(const stp_string_list_t *list,
			   const char *value);

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_STRING_LIST_H */
/*
 * End of "$Id: string-list.h,v 1.3 2006/07/07 22:49:14 rleigh Exp $".
 */
