/*
 *  FILE: table.c
 *  AUTH: Michael John Radwin <mjr@acm.org>
 *
 *  DESC: stubgen symbol table goodies
 *
 *  DATE: 1996/08/14 22:04:47
 *   $Id: table.c 10 2002-07-09 12:24:59Z ejakowatz $
 *
 *  Copyright (c) 1996-1998  Michael John Radwin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "table.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static const char rcsid[] = "$Id: table.c 10 2002-07-09 12:24:59Z ejakowatz $";

/* global table */
static syntaxelem_t etable[NELEMS];
static int eindex;

/* initializes all of the tables to default values. */
void init_tables()
{
  memset(etable, 0, sizeof(syntaxelem_t) * NELEMS);
  eindex = 0;

#if 0
    syntaxelem_t *elt;

    for(elt = etable; elt < &etable[NELEMS]; elt++) {
	elt->name = NULL;
	elt->ret_type = NULL;
	elt->args = NULL;
	elt->templ = NULL;
	elt->parent = NULL;
	elt->children = NULL;
	elt->throw_decl = NULL;
	elt->const_flag = 0;
	elt->kind = 73;
    }
#endif
}

/* recursively frees all syntaxelem_t's. */
void free_tables()
{
  syntaxelem_t *elt;
  
  for (elt = etable; elt < &etable[NELEMS]; elt++) {
    if (elt->name) {
      free(elt->name);
      free(elt->ret_type);
      free_args(elt->args);
      
      if (elt->throw_decl) free(elt->throw_decl);
      if (elt->templ) free(elt->templ);
    }
  }
}

/* recursively frees the argument list and assoicated fields */
void free_args(arg_t *args)
{
  arg_t *a;

  for (a = args; a != NULL; ) {
    arg_t *next_arg = a->next;
    free(a->type);
    if (a->name)
      free(a->name);
    if (a->array)
      free(a->array);
    free(a);
    a = next_arg;
  }
}

/* compares two arguments for equality.  Returns 0 for equal,
   non-zero if they differ. */
int arg_cmp(arg_t *a1, arg_t *a2)
{
  if (strcmp(a1->type, a2->type) != 0)
    return 1;
  
  if (a1->array) {
    if (a2->array) {
      if (strcmp(a1->array, a2->array) != 0)
	return 1;
    } else {
      return 1;
    }
  } else if (a2->array) {
    return 1;
  }

  return 0;
}

/* recursively compares two argument lists for equality.
   Returns 0 for equal, non-zero if they differ. */
int args_cmp(arg_t *a1, arg_t *a2)
{
  while (a1 != NULL) {
    if (a2 == NULL)
      return 1;
    
    if (arg_cmp(a1, a2) != 0)
      return 1;

    a1 = a1->next;
    a2 = a2->next;
  }

  if (a2 != NULL)
    return 1;

  return 0;
}


/* provides a string rep of this arglist.  conses up new
   memory, so client is responsible for freeing it. */
char * args_to_string(arg_t *args, int indent_length)
{
  arg_t *a;
  int len = 0;
  char *str;
  char *indent_str;

  if (indent_length <= 0) {
    indent_str = strdup(", ");
    indent_length = 2;

  } else {
    indent_length += 2;      /* room for ",\n" as well */
    indent_str = (char *) malloc(indent_length + 1);
    memset(indent_str, ' ', indent_length);
    indent_str[0] = ',';
    indent_str[1] = '\n';
    indent_str[indent_length] = '\0';
  }

  for (a = args; a != NULL; a = a->next) {
    len += strlen(a->type);  /* we always have a type */
    if (a->name)
      len += (1 + strlen(a->name));  /* room for ' ' and name */
    if (a->array)
      len += strlen(a->array);
    if (a != args)
      len += indent_length;  /* room for ", " */
  }

  str = (char *) malloc(len + 1);
  str[0] = '\0';

  for (a = args; a != NULL; a = a->next) {
    if (a != args)
      strcat(str, indent_str);

    strcat(str, a->type);
    if (a->name) {
      if (((a->type)[strlen(a->type)-1] != '*') &&
	  ((a->type)[strlen(a->type)-1] != '&'))
	strcat(str, " ");
      strcat(str, a->name);
      if (a->array)
	strcat(str, a->array);  /* array hugs variable name */
    } else {
      if (a->array)
	strcat(str, a->array);  /* array hugs type name */
    }
  }

  free(indent_str);
  return str;
}

/* compares a skeleton element to a header element for equality of
   signatures.  this isn't a strict comparison, because the kind and
   parent fields ought to be different.  returns 0 if equal, non-zero if
   different */
int skel_elemcmp(syntaxelem_t *skel_elem, syntaxelem_t *hdr_elem)
{
  char *tmp_str;
  int result;

  assert(hdr_elem->kind == FUNC_KIND);
  assert(skel_elem->kind == SKEL_KIND);
  assert(hdr_elem->parent != NULL);

  if (skel_elem->const_flag != hdr_elem->const_flag)
    return 1;
  
  /* 
   * templ and throw_decl are allowed to be NULL,
   * so we must not pass them onto strcmp without
   * a check first.
   */
  if (skel_elem->templ != NULL) {
    if (hdr_elem->templ == NULL)
      return 1;
    else if (strcmp(skel_elem->templ, hdr_elem->templ) != 0)
      return 1;
  } else if (hdr_elem->templ != NULL) {
    return 1;
  }
  
  if (skel_elem->throw_decl != NULL) {
    if (hdr_elem->throw_decl == NULL)
      return 1;
    else if (strcmp(skel_elem->throw_decl, hdr_elem->throw_decl) != 0)
      return 1;
  } else if (hdr_elem->throw_decl != NULL) {
    return 1;
  }

  /*
   * these two won't differ across files, and they're
   * guaranteed not to be NULL.
   */
  if (strcmp(skel_elem->ret_type, hdr_elem->ret_type) != 0)
    return 1;

  /* now make sure the argument signatures match */
  if (args_cmp(skel_elem->args, hdr_elem->args) != 0)
    return 1;
  
  /* 
   * the name, of course, is the hard part.  we gotta
   * look at the parent to make a scoped name.
   */
  tmp_str = (char *) malloc(strlen(hdr_elem->parent->name) +
			    strlen(hdr_elem->name) + 3);
  sprintf(tmp_str, "%s::%s", hdr_elem->parent->name, hdr_elem->name);

  result = strcmp(skel_elem->name, tmp_str);
  free(tmp_str);

  return result;
}

/* allocates a new element from the table, filling in the apropriate fields */
syntaxelem_t * new_elem(char *ret_type, char *name, arg_t *args, int kind)
{
    syntaxelem_t *se;

    if (eindex == NELEMS - 1) {
	fatal(2, "Too many symbols.  Please mail mjr@acm.org.");
	return NULL;
    }

    se = &etable[eindex++];
    se->ret_type = ret_type;
    se->name = name;
    se->args = args;
    se->kind = kind;

    return se;
}


/* given the head of the list, reverses it and returns the new head */
syntaxelem_t * reverse_list(syntaxelem_t *head) 
{
    syntaxelem_t *elt = head;
    syntaxelem_t *prev = NULL;
    syntaxelem_t *next;

    while (elt != NULL) {
	next = elt->next;
	elt->next = prev;
	prev = elt;
	elt = next;
    }
  
    return prev;
}

arg_t * reverse_arg_list(arg_t *head) 
{
  arg_t *a = head;
  arg_t *prev = NULL;
  arg_t *next;

  while (a != NULL) {
    next = a->next;
    a->next = prev;
    prev = a;
    a = next;
  }
  
  return prev;
}
    
const char * string_kind(int some_KIND)
{
  switch(some_KIND) {
  case IGNORE_KIND:
    return "IGNORE_KIND";
  case FUNC_KIND:
    return "FUNC_KIND";
  case CLASS_KIND:
    return "CLASS_KIND";
  case STRUCT_KIND:
    return "STRUCT_KIND";
  case INLINED_KIND:
    return "INLINED_KIND";
  case SKEL_KIND:
    return "SKEL_KIND";
  case DONE_FUNC_KIND:
    return "DONE_FUNC_KIND";
  case DONE_CLASS_KIND:
    return "DONE_CLASS_KIND";
  default:
    return "BAD KIND!";
  }
}


#ifdef SGDEBUG
void print_se(syntaxelem_t *elt)
{
  char *arg_str = args_to_string(elt->args, 0);

  log_printf("\nSTUBELEM name: %s\n", elt->name);
  log_printf("      ret_typ: %s\n", elt->ret_type);
  log_printf("         args: %s\n", arg_str);
  log_printf("       parent: %s\n", (elt->parent) ? elt->parent->name : "NULL");
  log_printf("         next: %s\n", (elt->next) ? elt->next->name : "NULL");
  log_printf("        const: %d\n", elt->const_flag);
  log_printf("         kind: %s\n", string_kind(elt->kind));
  log_printf("        throw: %s\n", (elt->throw_decl)? elt->throw_decl : "NULL");
  log_printf("        templ: %s\n\n", (elt->templ)? elt->templ : "NULL");
  free(arg_str);
}
#endif /* SGDEBUG */

/*
 * we can't use the 'next' field of syntaxelem_t because it is used to
 * chain together methods.  Make a slightly bigger struct so we can
 * queue these up. 
 */
typedef struct _skelnode {
    syntaxelem_t *elt;
    struct _skelnode *next;
} skelnode_t;

/* the queue of elements found in the .H file, possibly to be expanded */
static skelnode_t *exp_head = NULL;
static skelnode_t *exp_tail = NULL;

void enqueue_class(syntaxelem_t *elt)
{
    skelnode_t *new_class;

    new_class = (skelnode_t *)malloc(sizeof(skelnode_t));
    new_class->elt = elt;
    new_class->next = NULL;

    if (exp_head == NULL)
	exp_tail = exp_head = new_class;
    else {
	exp_tail->next = new_class;
	exp_tail = new_class;
    }
}

syntaxelem_t * dequeue_class()
{
    skelnode_t *old_head = exp_head;
    syntaxelem_t *to_return = exp_head->elt;

    assert(exp_head != NULL);
    exp_head = exp_head->next;
    free(old_head);

    return to_return;
}

int class_queue_empty()
{
    return exp_head == NULL;
}

/* the list of skeletons found in the .C file, already expanded */
static skelnode_t *skel_head = NULL;

void enqueue_skeleton(syntaxelem_t *elt)
{
  skelnode_t *new_class;

  new_class = (skelnode_t *)malloc(sizeof(skelnode_t));
  new_class->elt = elt;
  new_class->next = skel_head;

  skel_head = new_class;
}

syntaxelem_t * find_skeleton(syntaxelem_t *elt)
{
  skelnode_t *node;

  /* the order of skel_elemcmp(node->elt, elt) is important. */
  for (node = skel_head; node != NULL; node = node->next)
    if (skel_elemcmp(node->elt, elt) == 0)
      return node->elt;
  
  return NULL;
}

void clear_skeleton_queue()
{
  skelnode_t *node = skel_head;

  while (node != NULL) {
    skelnode_t *next_node = node->next;
    free(node);
    node = next_node;
  }

  skel_head = NULL;
}
