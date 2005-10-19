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

#include "system.h"

#include "complain.h"
#include "symlist.h"


/*--------------------------------------.
| Create a list containing SYM at LOC.  |
`--------------------------------------*/

symbol_list *
symbol_list_new (symbol *sym, location loc)
{
  symbol_list *res = xmalloc (sizeof *res);
  res->next = NULL;
  res->sym = sym;
  res->location = loc;
  res->action = NULL;
  res->ruleprec = NULL;
  res->dprec = 0;
  res->merger = 0;
  return res;
}


/*------------------.
| Print this list.  |
`------------------*/

void
symbol_list_print (symbol_list *l, FILE *f)
{
  for (/* Nothing. */; l && l->sym; l = l->next)
    {
      symbol_print (l->sym, f);
      if (l && l->sym)
	fputc (' ', f);
    }
}


/*---------------------------------.
| Prepend SYM at LOC to the LIST.  |
`---------------------------------*/

symbol_list *
symbol_list_prepend (symbol_list *list, symbol *sym, location loc)
{
  symbol_list *res = symbol_list_new (sym, loc);
  res->next = list;
  return res;
}


/*-------------------------------------------------.
| Free the LIST, but not the symbols it contains.  |
`-------------------------------------------------*/

void
symbol_list_free (symbol_list *list)
{
  LIST_FREE (symbol_list, list);
}


/*--------------------.
| Return its length.  |
`--------------------*/

unsigned int
symbol_list_length (symbol_list *list)
{
  int res = 0;
  for (/* Nothing. */; list; list = list->next)
    ++res;
  return res;
}


/*--------------------------------------------------------------.
| Get the data type (alternative in the union) of the value for |
| symbol N in symbol list RP.                                   |
`--------------------------------------------------------------*/

uniqstr
symbol_list_n_type_name_get (symbol_list *rp, location loc, int n)
{
  int i;

  if (n < 0)
    {
      complain_at (loc, _("invalid $ value: $%d"), n);
      return NULL;
    }

  i = 0;

  while (i < n)
    {
      rp = rp->next;
      if (rp == NULL || rp->sym == NULL)
	{
	  complain_at (loc, _("invalid $ value: $%d"), n);
	  return NULL;
	}
      ++i;
    }

  return rp->sym->type_name;
}
