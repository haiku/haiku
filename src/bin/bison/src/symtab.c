/* Symbol table manager for Bison.

   Copyright (C) 1984, 1989, 2000, 2001, 2002, 2004 Free Software
   Foundation, Inc.

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#include "system.h"

#include <hash.h>
#include <quotearg.h>

#include "complain.h"
#include "gram.h"
#include "symtab.h"

/*------------------------.
| Distinguished symbols.  |
`------------------------*/

symbol *errtoken = NULL;
symbol *undeftoken = NULL;
symbol *endtoken = NULL;
symbol *accept = NULL;
symbol *startsymbol = NULL;
location startsymbol_location;

/*---------------------------------.
| Create a new symbol, named TAG.  |
`---------------------------------*/

static symbol *
symbol_new (uniqstr tag, location loc)
{
  symbol *res = MALLOC (res, 1);

  uniqstr_assert (tag);
  res->tag = tag;
  res->location = loc;

  res->type_name = NULL;
  res->destructor = NULL;
  res->printer = NULL;

  res->number = NUMBER_UNDEFINED;
  res->prec = 0;
  res->assoc = undef_assoc;
  res->user_token_number = USER_NUMBER_UNDEFINED;

  res->alias = NULL;
  res->class = unknown_sym;

  nsyms++;
  return res;
}


/*-----------------------------------------------------------------.
| Set the TYPE_NAME associated with SYM.  Does nothing if passed 0 |
| as TYPE_NAME.                                                    |
`-----------------------------------------------------------------*/

void
symbol_type_set (symbol *sym, uniqstr type_name, location loc)
{
  if (type_name)
    {
      if (sym->type_name)
	complain_at (loc, _("type redeclaration for %s"), sym->tag);
      uniqstr_assert (type_name);
      sym->type_name = type_name;
    }
}


/*------------------------------------------------------------------.
| Set the DESTRUCTOR associated with SYM.  Do nothing if passed 0.  |
`------------------------------------------------------------------*/

void
symbol_destructor_set (symbol *sym, char *destructor, location loc)
{
  if (destructor)
    {
      if (sym->destructor)
	complain_at (loc, _("%s redeclaration for %s"),
		     "%destructor", sym->tag);
      sym->destructor = destructor;
      sym->destructor_location = loc;
    }
}


/*---------------------------------------------------------------.
| Set the PRINTER associated with SYM.  Do nothing if passed 0.  |
`---------------------------------------------------------------*/

void
symbol_printer_set (symbol *sym, char *printer, location loc)
{
  if (printer)
    {
      if (sym->printer)
	complain_at (loc, _("%s redeclaration for %s"),
		     "%printer", sym->tag);
      sym->printer = printer;
      sym->printer_location = loc;
    }
}


/*-----------------------------------------------------------------.
| Set the PRECEDENCE associated with SYM.  Does nothing if invoked |
| with UNDEF_ASSOC as ASSOC.                                       |
`-----------------------------------------------------------------*/

void
symbol_precedence_set (symbol *sym, int prec, assoc a, location loc)
{
  if (a != undef_assoc)
    {
      if (sym->prec != 0)
	complain_at (loc, _("redefining precedence of %s"), sym->tag);
      sym->prec = prec;
      sym->assoc = a;
    }

  /* Only terminals have a precedence. */
  symbol_class_set (sym, token_sym, loc);
}


/*------------------------------------.
| Set the CLASS associated with SYM.  |
`------------------------------------*/

void
symbol_class_set (symbol *sym, symbol_class class, location loc)
{
  if (sym->class != unknown_sym && sym->class != class)
    complain_at (loc, _("symbol %s redefined"), sym->tag);

  if (class == nterm_sym && sym->class != nterm_sym)
    sym->number = nvars++;
  else if (class == token_sym && sym->number == NUMBER_UNDEFINED)
    sym->number = ntokens++;

  sym->class = class;
}


/*------------------------------------------------.
| Set the USER_TOKEN_NUMBER associated with SYM.  |
`------------------------------------------------*/

void
symbol_user_token_number_set (symbol *sym, int user_token_number, location loc)
{
  if (sym->class != token_sym)
    abort ();

  if (sym->user_token_number != USER_NUMBER_UNDEFINED
      && sym->user_token_number != user_token_number)
    complain_at (loc, _("redefining user token number of %s"), sym->tag);

  sym->user_token_number = user_token_number;
  /* User defined $end token? */
  if (user_token_number == 0)
    {
      endtoken = sym;
      endtoken->number = 0;
      /* It is always mapped to 0, so it was already counted in
	 NTOKENS.  */
      --ntokens;
    }
}


/*----------------------------------------------------------.
| If SYM is not defined, report an error, and consider it a |
| nonterminal.                                              |
`----------------------------------------------------------*/

static inline bool
symbol_check_defined (symbol *sym)
{
  if (sym->class == unknown_sym)
    {
      complain_at
	(sym->location,
	 _("symbol %s is used, but is not defined as a token and has no rules"),
	 sym->tag);
      sym->class = nterm_sym;
      sym->number = nvars++;
    }

  return true;
}

static bool
symbol_check_defined_processor (void *sym, void *null ATTRIBUTE_UNUSED)
{
  return symbol_check_defined (sym);
}


/*------------------------------------------------------------------.
| Declare the new symbol SYM.  Make it an alias of SYMVAL, and type |
| SYMVAL with SYM's type.                                           |
`------------------------------------------------------------------*/

void
symbol_make_alias (symbol *sym, symbol *symval, location loc)
{
  if (symval->alias)
    warn_at (loc, _("symbol `%s' used more than once as a literal string"),
	  symval->tag);
  else if (sym->alias)
    warn_at (loc, _("symbol `%s' given more than one literal string"),
	  sym->tag);
  else
    {
      symval->class = token_sym;
      symval->user_token_number = sym->user_token_number;
      sym->user_token_number = USER_NUMBER_ALIAS;
      symval->alias = sym;
      sym->alias = symval;
      /* sym and symval combined are only one symbol.  */
      nsyms--;
      ntokens--;
      if (ntokens != sym->number && ntokens != symval->number)
	abort ();
      sym->number = symval->number =
	(symval->number < sym->number) ? symval->number : sym->number;
      symbol_type_set (symval, sym->type_name, loc);
    }
}


/*---------------------------------------------------------.
| Check that THIS, and its alias, have same precedence and |
| associativity.                                           |
`---------------------------------------------------------*/

static inline bool
symbol_check_alias_consistency (symbol *this)
{
  /* Check only those who _are_ the aliases. */
  if (this->alias && this->user_token_number == USER_NUMBER_ALIAS)
    {
      if (this->prec != this->alias->prec)
	{
	  if (this->prec != 0 && this->alias->prec != 0)
	    complain_at (this->alias->location,
			 _("conflicting precedences for %s and %s"),
			 this->tag, this->alias->tag);
	  if (this->prec != 0)
	    this->alias->prec = this->prec;
	  else
	    this->prec = this->alias->prec;
	}

      if (this->assoc != this->alias->assoc)
	{
	  if (this->assoc != undef_assoc && this->alias->assoc != undef_assoc)
	    complain_at (this->alias->location,
			 _("conflicting associativities for %s (%s) and %s (%s)"),
			 this->tag, assoc_to_string (this->assoc),
			 this->alias->tag, assoc_to_string (this->alias->assoc));
	  if (this->assoc != undef_assoc)
	    this->alias->assoc = this->assoc;
	  else
	    this->assoc = this->alias->assoc;
	}
    }
  return true;
}

static bool
symbol_check_alias_consistency_processor (void *this,
					  void *null ATTRIBUTE_UNUSED)
{
  return symbol_check_alias_consistency (this);
}


/*-------------------------------------------------------------------.
| Assign a symbol number, and write the definition of the token name |
| into FDEFINES.  Put in SYMBOLS.                                    |
`-------------------------------------------------------------------*/

static inline bool
symbol_pack (symbol *this)
{
  if (this->class == nterm_sym)
    {
      this->number += ntokens;
    }
  else if (this->alias)
    {
      /* This symbol and its alias are a single token defn.
	 Allocate a tokno, and assign to both check agreement of
	 prec and assoc fields and make both the same */
      if (this->number == NUMBER_UNDEFINED)
	{
	  if (this == endtoken || this->alias == endtoken)
	    this->number = this->alias->number = 0;
	  else
	    {
	      if (this->alias->number == NUMBER_UNDEFINED)
		abort ();
	      this->number = this->alias->number;
	    }
	}
      /* Do not do processing below for USER_NUMBER_ALIASes.  */
      if (this->user_token_number == USER_NUMBER_ALIAS)
	return true;
    }
  else /* this->class == token_sym */
    {
      if (this->number == NUMBER_UNDEFINED)
	abort ();
    }

  symbols[this->number] = this;
  return true;
}

static bool
symbol_pack_processor (void *this, void *null ATTRIBUTE_UNUSED)
{
  return symbol_pack (this);
}




/*--------------------------------------------------.
| Put THIS in TOKEN_TRANSLATIONS if it is a token.  |
`--------------------------------------------------*/

static inline bool
symbol_translation (symbol *this)
{
  /* Non-terminal? */
  if (this->class == token_sym
      && this->user_token_number != USER_NUMBER_ALIAS)
    {
      /* A token which translation has already been set? */
      if (token_translations[this->user_token_number] != undeftoken->number)
	complain_at (this->location,
		     _("tokens %s and %s both assigned number %d"),
		     symbols[token_translations[this->user_token_number]]->tag,
		     this->tag, this->user_token_number);

      token_translations[this->user_token_number] = this->number;
    }

  return true;
}

static bool
symbol_translation_processor (void *this, void *null ATTRIBUTE_UNUSED)
{
  return symbol_translation (this);
}


/*----------------------.
| A symbol hash table.  |
`----------------------*/

/* Initial capacity of symbols hash table.  */
#define HT_INITIAL_CAPACITY 257

static struct hash_table *symbol_table = NULL;

static inline bool
hash_compare_symbol (const symbol *m1, const symbol *m2)
{
  /* Since tags are unique, we can compare the pointers themselves.  */
  return UNIQSTR_EQ (m1->tag, m2->tag);
}

static bool
hash_symbol_comparator (void const *m1, void const *m2)
{
  return hash_compare_symbol (m1, m2);
}

static inline size_t
hash_symbol (const symbol *m, size_t tablesize)
{
  /* Since tags are unique, we can hash the pointer itself.  */
  return ((uintptr_t) m->tag) % tablesize;
}

static size_t
hash_symbol_hasher (void const *m, size_t tablesize)
{
  return hash_symbol (m, tablesize);
}


/*-------------------------------.
| Create the symbol hash table.  |
`-------------------------------*/

void
symbols_new (void)
{
  symbol_table = hash_initialize (HT_INITIAL_CAPACITY,
				  NULL,
				  hash_symbol_hasher,
				  hash_symbol_comparator,
				  free);
}


/*----------------------------------------------------------------.
| Find the symbol named KEY, and return it.  If it does not exist |
| yet, create it.                                                 |
`----------------------------------------------------------------*/

symbol *
symbol_get (const char *key, location loc)
{
  symbol probe;
  symbol *entry;

  /* Keep the symbol in a printable form.  */
  key = uniqstr_new (quotearg_style (escape_quoting_style, key));
  probe.tag = key;
  entry = hash_lookup (symbol_table, &probe);

  if (!entry)
    {
      /* First insertion in the hash. */
      entry = symbol_new (key, loc);
      hash_insert (symbol_table, entry);
    }
  return entry;
}


/*------------------------------------------------------------------.
| Generate a dummy nonterminal, whose name cannot conflict with the |
| user's names.                                                     |
`------------------------------------------------------------------*/

symbol *
dummy_symbol_get (location loc)
{
  /* Incremented for each generated symbol.  */
  static int dummy_count = 0;
  static char buf[256];

  symbol *sym;

  sprintf (buf, "@%d", ++dummy_count);
  sym = symbol_get (buf, loc);
  sym->class = nterm_sym;
  sym->number = nvars++;
  return sym;
}


/*-------------------.
| Free the symbols.  |
`-------------------*/

void
symbols_free (void)
{
  hash_free (symbol_table);
  free (symbols);
}


/*---------------------------------------------------------------.
| Look for undefined symbols, report an error, and consider them |
| terminals.                                                     |
`---------------------------------------------------------------*/

static void
symbols_do (Hash_processor processor, void *processor_data)
{
  hash_do_for_each (symbol_table, processor, processor_data);
}


/*--------------------------------------------------------------.
| Check that all the symbols are defined.  Report any undefined |
| symbols and consider them nonterminals.                       |
`--------------------------------------------------------------*/

void
symbols_check_defined (void)
{
  symbols_do (symbol_check_defined_processor, NULL);
}

/*------------------------------------------------------------------.
| Set TOKEN_TRANSLATIONS.  Check that no two symbols share the same |
| number.                                                           |
`------------------------------------------------------------------*/

static void
symbols_token_translations_init (void)
{
  bool num_256_available_p = true;
  int i;

  /* Find the highest user token number, and whether 256, the POSIX
     preferred user token number for the error token, is used.  */
  max_user_token_number = 0;
  for (i = 0; i < ntokens; ++i)
    {
      symbol *this = symbols[i];
      if (this->user_token_number != USER_NUMBER_UNDEFINED)
	{
	  if (this->user_token_number > max_user_token_number)
	    max_user_token_number = this->user_token_number;
	  if (this->user_token_number == 256)
	    num_256_available_p = false;
	}
    }

  /* If 256 is not used, assign it to error, to follow POSIX.  */
  if (num_256_available_p
      && errtoken->user_token_number == USER_NUMBER_UNDEFINED)
    errtoken->user_token_number = 256;

  /* Set the missing user numbers. */
  if (max_user_token_number < 256)
    max_user_token_number = 256;

  for (i = 0; i < ntokens; ++i)
    {
      symbol *this = symbols[i];
      if (this->user_token_number == USER_NUMBER_UNDEFINED)
	this->user_token_number = ++max_user_token_number;
      if (this->user_token_number > max_user_token_number)
	max_user_token_number = this->user_token_number;
    }

  CALLOC (token_translations, max_user_token_number + 1);

  /* Initialize all entries for literal tokens to 2, the internal
     token number for $undefined, which represents all invalid inputs.
     */
  for (i = 0; i < max_user_token_number + 1; i++)
    token_translations[i] = undeftoken->number;
  symbols_do (symbol_translation_processor, NULL);
}


/*----------------------------------------------------------------.
| Assign symbol numbers, and write definition of token names into |
| FDEFINES.  Set up vectors SYMBOL_TABLE, TAGS of symbols.        |
`----------------------------------------------------------------*/

void
symbols_pack (void)
{
  CALLOC (symbols, nsyms);

  symbols_do (symbol_check_alias_consistency_processor, NULL);
  symbols_do (symbol_pack_processor, NULL);

  symbols_token_translations_init ();

  if (startsymbol->class == unknown_sym)
    fatal_at (startsymbol_location,
	      _("the start symbol %s is undefined"),
	      startsymbol->tag);
  else if (startsymbol->class == token_sym)
    fatal_at (startsymbol_location,
	      _("the start symbol %s is a token"),
	      startsymbol->tag);
}
