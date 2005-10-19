/* Lists of symbols for Bison

   Copyright (C) 2002, 2005 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   Bison is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bison is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bison; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#ifndef SYMLIST_H_
# define SYMLIST_H_

# include "location.h"
# include "symtab.h"

typedef struct symbol_list
{
  struct symbol_list *next;
  symbol *sym;
  location location;

  /* The action is attached to the LHS of a rule. */
  const char *action;
  location action_location;

  symbol *ruleprec;
  int dprec;
  int merger;
} symbol_list;


/* Create a list containing SYM at LOC.  */
symbol_list *symbol_list_new (symbol *sym, location loc);

/* Print it.  */
void symbol_list_print (symbol_list *l, FILE *f);

/* Prepend SYM at LOC to the LIST.  */
symbol_list *symbol_list_prepend (symbol_list *list,
				  symbol *sym,
				  location loc);

/* Free the LIST, but not the symbols it contains.  */
void symbol_list_free (symbol_list *list);

/* Return its length. */
unsigned int symbol_list_length (symbol_list *list);

/* Get the data type (alternative in the union) of the value for
   symbol N in rule RULE.  */
uniqstr symbol_list_n_type_name_get (symbol_list *rp, location loc, int n);

#endif /* !SYMLIST_H_ */
