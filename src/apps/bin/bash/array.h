/* array.h -- definitions for the interface exported by array.c that allows
   the rest of the shell to manipulate array variables. */

/* Copyright (C) 1997 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#ifndef _ARRAY_H_
#define _ARRAY_H_

#include "stdc.h"

typedef intmax_t	arrayind_t;

enum atype {array_indexed, array_assoc};

typedef struct array {
	enum atype	type;
	arrayind_t	max_index, num_elements;
	struct array_element *head;
} ARRAY;

typedef struct array_element {
	arrayind_t	ind;
	char	*value;
	struct array_element *next, *prev;
} ARRAY_ELEMENT;

typedef int sh_ae_map_func_t __P((ARRAY_ELEMENT *));

/* Basic operations on entire arrays */
extern ARRAY	*array_create __P((void));
extern void	array_flush __P((ARRAY *));
extern void	array_dispose __P((ARRAY *));
extern ARRAY	*array_copy __P((ARRAY *));
extern ARRAY	*array_slice __P((ARRAY *, ARRAY_ELEMENT *, ARRAY_ELEMENT *));
extern void	array_walk __P((ARRAY   *, sh_ae_map_func_t *));

extern ARRAY_ELEMENT *array_shift __P((ARRAY *, int, int));
extern int	array_rshift __P((ARRAY *, int, char *));
extern ARRAY	*array_quote __P((ARRAY *));

extern char	*array_subrange __P((ARRAY *, arrayind_t, arrayind_t, int));
extern char	*array_patsub __P((ARRAY *, char *, char *, int));

/* Basic operations on array elements. */
extern ARRAY_ELEMENT *array_create_element __P((arrayind_t, char *));
extern ARRAY_ELEMENT *array_copy_element __P((ARRAY_ELEMENT *));
extern void	array_dispose_element __P((ARRAY_ELEMENT *));

extern int	array_insert __P((ARRAY *, arrayind_t, char *));
extern ARRAY_ELEMENT *array_remove __P((ARRAY *, arrayind_t));
extern char	*array_reference __P((ARRAY *, arrayind_t));

/* Converting to and from arrays */
extern WORD_LIST *array_to_word_list __P((ARRAY *));
extern ARRAY *array_from_word_list __P((WORD_LIST *));
extern ARRAY *array_assign_list __P((ARRAY *, WORD_LIST *));

extern char **array_to_argv __P((ARRAY *));

extern char *array_to_assign __P((ARRAY *, int));
extern char *array_to_string __P((ARRAY *, char *, int));
extern ARRAY *array_from_string __P((char *, char *));

/* Flags for array_shift */
#define AS_DISPOSE	0x01

#define array_num_elements(a)	((a)->num_elements)
#define array_max_index(a)	((a)->max_index)
#define array_head(a)		((a)->head)
#define array_empty(a)		((a)->num_elements == 0)

#define element_value(ae)	((ae)->value)
#define element_index(ae)	((ae)->ind)
#define element_forw(ae)	((ae)->next)
#define element_back(ae)	((ae)->prev)

#define ALL_ELEMENT_SUB(c)	((c) == '@' || (c) == '*')

#endif /* _ARRAY_H_ */
