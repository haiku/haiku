/*
 * "$Id: print-list.c,v 1.25 2010/08/04 00:33:57 rlk Exp $"
 *
 *   Gutenprint list functions.  A doubly-linked list implementation,
 *   with callbacks for freeing, sorting, and retrieving nodes by name
 *   or long name.
 *
 *   Copyright 2002 Roger Leigh (rleigh@debian.org)
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, etc.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/** The internal representation of an stp_list_item_t list node. */
struct stp_list_item
{
  void *data;			/*!< Data		*/
  struct stp_list_item *prev;	/*!< Previous node	*/
  struct stp_list_item *next;	/*!< Next node		*/
};

/** The internal representation of an stp_list_t list. */
struct stp_list
{
  int index_cache;				/*!< Cached node index			*/
  struct stp_list_item *start;			/*!< Start node				*/
  struct stp_list_item *end;			/*!< End node				*/
  struct stp_list_item *index_cache_node;	/*!< Cached node (for index)		*/
  int length;					/*!< Number of nodes			*/
  stp_node_freefunc freefunc;			/*!< Callback to free node data		*/
  stp_node_copyfunc copyfunc;			/*!< Callback to copy node		*/
  stp_node_namefunc namefunc;			/*!< Callback to get node name		*/
  stp_node_namefunc long_namefunc;		/*!< Callback to get node long name	*/
  stp_node_sortfunc sortfunc;			/*!< Callback to compare (sort) nodes	*/
  char *name_cache;				/*!< Cached name			*/
  struct stp_list_item *name_cache_node;	/*!< Cached node (for name)		*/
  char *long_name_cache;			/*!< Cached long name			*/
  struct stp_list_item *long_name_cache_node;	/*!< Cached node (for long name)	*/
};

/**
 * Cache a list node by its short name.
 * @param list the list to use.
 * @param name the short name.
 * @param cache the node to cache.
 */
static void
set_name_cache(stp_list_t *list,
	       const char *name,
	       stp_list_item_t *cache)
{
  if (list->name_cache)
    stp_free(list->name_cache);
  list->name_cache = NULL;
  if (name)
    list->name_cache = stp_strdup(name);
  list->name_cache_node = cache;
}

/**
 * Cache a list node by its long name.
 * @param list the list to use.
 * @param long_name the long name.
 * @param cache the node to cache.
 */
static void
set_long_name_cache(stp_list_t *list,
		    const char *long_name,
		    stp_list_item_t *cache)
{
  if (list->long_name_cache)
    stp_free(list->long_name_cache);
  list->long_name_cache = NULL;
  if (long_name)
    list->long_name_cache = stp_strdup(long_name);
  list->long_name_cache_node = cache;
}

/**
 * Clear cached nodes.
 * @param list the list to use.
 */
static inline void
clear_cache(stp_list_t *list)
{
  list->index_cache = 0;
  list->index_cache_node = NULL;
  set_name_cache(list, NULL, NULL);
  set_long_name_cache(list, NULL, NULL);
}

void
stp_list_node_free_data (void *item)
{
  stp_free(item);
  stp_deprintf(STP_DBG_LIST, "stp_list_node_free_data destructor\n");
}

/** Check the validity of a list. */
#define check_list(List) STPI_ASSERT(List != NULL, NULL)

/* List head functions.
 * These functions operate on the list as a whole, and not the
 * individual nodes in a list. */

/* create a new list */
stp_list_t *
stp_list_create(void)
{
  stp_list_t *list =
    stp_malloc(sizeof(stp_list_t));

  /* initialise an empty list */
  list->index_cache = 0;
  list->length = 0;
  list->start = NULL;
  list->end = NULL;
  list->index_cache_node = NULL;
  list->freefunc = NULL;
  list->namefunc = NULL;
  list->long_namefunc = NULL;
  list->sortfunc = NULL;
  list->copyfunc = NULL;
  list->name_cache = NULL;
  list->name_cache_node = NULL;
  list->long_name_cache = NULL;
  list->long_name_cache_node = NULL;

  stp_deprintf(STP_DBG_LIST, "stp_list_head constructor\n");
  return list;
}

stp_list_t *
stp_list_copy(const stp_list_t *list)
{
  stp_list_t *ret;
  stp_node_copyfunc copyfunc = stp_list_get_copyfunc(list);
  stp_list_item_t *item = list->start;

  check_list(list);

  ret = stp_list_create();
  stp_list_set_copyfunc(ret, stp_list_get_copyfunc(list));
  /* If we use default (shallow) copy, we can't free the elements of it */
  if (stp_list_get_copyfunc(list))
    stp_list_set_freefunc(ret, stp_list_get_freefunc(list));
  stp_list_set_namefunc(ret, stp_list_get_namefunc(list));
  stp_list_set_long_namefunc(ret, stp_list_get_long_namefunc(list));
  stp_list_set_sortfunc(ret, stp_list_get_sortfunc(list));
  while (item)
    {
      void *data = item->data;
      if (copyfunc)
	stp_list_item_create (ret, NULL, (*copyfunc)(data));
      else
	stp_list_item_create(ret, NULL, data);
      item = stp_list_item_next(item);
    }
  return ret;
}

/* free a list, freeing all child nodes first */
int
stp_list_destroy(stp_list_t *list)
{
  stp_list_item_t *cur;
  stp_list_item_t *next;

  check_list(list);
  clear_cache(list);
  cur = list->start;
  while(cur)
    {
      next = cur->next;
      stp_list_item_destroy(list, cur);
      cur = next;
    }
  stp_deprintf(STP_DBG_LIST, "stp_list_head destructor\n");
  stp_free(list);

  return 0;
}

int
stp_list_get_length(const stp_list_t *list)
{
  check_list(list);
  return list->length;
}

/* find a node */

/* get the first node in the list */

stp_list_item_t *
stp_list_get_start(const stp_list_t *list)
{
  return list->start;
}

/* get the last node in the list */

stp_list_item_t *
stp_list_get_end(const stp_list_t *list)
{
  return list->end;
}

static inline stp_list_t *
deconst_list(const stp_list_t *list)
{
  return (stp_list_t *) list;
}

/* get the node by its place in the list */
stp_list_item_t *
stp_list_get_item_by_index(const stp_list_t *list, int idx)
{
  stp_list_item_t *node = NULL;
  stp_list_t *ulist = deconst_list(list);
  int i; /* current index */
  int d = 0; /* direction of list traversal, 0=forward */
  int c = 0; /* use cache? */
  check_list(list);

  if (idx >= list->length)
    return NULL;

  /* see if using the cache is worthwhile */
  if (list->index_cache)
    {
      if (idx < (list->length/2))
	{
	  if (idx > abs(idx - list->index_cache))
	    c = 1;
	  else
	    d = 0;
	}
      else
	{
	  if (list->length - 1 - idx >
	      abs (list->length - 1 - idx - list->index_cache))
	    c = 1;
	  else
	    d = 1;
	}
    }


  if (c) /* use the cached index and node */
    {
      if (idx > list->index_cache) /* forward */
	d = 0;
      else /* backward */
	d = 1;
      i = list->index_cache;
      node = list->index_cache_node;
    }
  else /* start from one end of the list */
    {
      if (d)
	{
	  i = list->length - 1;
	  node = list->end;
	}
      else
	{
	  i = 0;
	  node = list->start;
	}
    }

  while (node && i != idx)
    {
      if (d)
	{
	  i--;
	  node = node->prev;
	}
      else
	{
	  i++;
	  node = node->next;
	}
    }

  /* update cache */
  ulist->index_cache = i;
  ulist->index_cache_node = node;

  return node;
}

/**
 * Find an item in a list by its name.
 * This internal helper is not optimised to use any caching.
 * @param list the list to use.
 * @param name the name to find.
 * @returns a pointer to the list item, or NULL if the name is
 * invalid or the list is empty.
 */
static stp_list_item_t *
stp_list_get_item_by_name_internal(const stp_list_t *list, const char *name)
{
  stp_list_item_t *node = list->start;
  while (node && strcmp(name, list->namefunc(node->data)))
    {
      node = node->next;
    }
  return node;
}

/* get the first node with name; requires a callback function to
   read data */
stp_list_item_t *
stp_list_get_item_by_name(const stp_list_t *list, const char *name)
{
  stp_list_item_t *node = NULL;
  stp_list_t *ulist = deconst_list(list);
  check_list(list);

  if (!list->namefunc)
    return NULL;

  if (list->name_cache && name && list->name_cache_node)
    {
      const char *new_name;
      node = list->name_cache_node;
      /* Is this the item we've cached? */
      if (strcmp(name, list->name_cache) == 0 &&
	  strcmp(name, list->namefunc(node->data)) == 0)
	return node;

      /* If not, check the next item in case we're searching the list */
      node = node->next;
      if (node)
	{
	  new_name = list->namefunc(node->data);
	  if (strcmp(name, new_name) == 0)
	    {
	      set_name_cache(ulist, new_name, node);
	      return node;
	    }
	}
      /* If not, check the index cache */
      node = list->index_cache_node;
      if (node)
	{
	  new_name = list->namefunc(node->data);
	  if (strcmp(name, new_name) == 0)
	    {
	      set_name_cache(ulist, new_name, node);
	      return node;
	    }
	}
    }

  node = stp_list_get_item_by_name_internal(list, name);

  if (node)
    set_name_cache(ulist, name, node);

  return node;
}


/**
 * Find an item in a list by its long name.
 * This internal helper is not optimised to use any caching.
 * @param list the list to use.
 * @param long_name the long name to find.
 * @returns a pointer to the list item, or NULL if the long name is
 * invalid or the list is empty.
 */
static stp_list_item_t *
stp_list_get_item_by_long_name_internal(const stp_list_t *list,
					 const char *long_name)
{
  stp_list_item_t *node = list->start;
  while (node && strcmp(long_name, list->long_namefunc(node->data)))
    {
      node = node->next;
    }
  return node;
}

/* get the first node with long_name; requires a callack function to
   read data */
stp_list_item_t *
stp_list_get_item_by_long_name(const stp_list_t *list, const char *long_name)
{
  stp_list_item_t *node = NULL;
  stp_list_t *ulist = deconst_list(list);
  check_list(list);

  if (!list->long_namefunc)
    return NULL;

  if (list->long_name_cache && long_name && list->long_name_cache_node)
    {
      const char *new_long_name;
      node = list->long_name_cache_node;
      /* Is this the item we've cached? */
      if (strcmp(long_name, list->long_name_cache) == 0 &&
	  strcmp(long_name, list->long_namefunc(node->data)) == 0)
	return node;

      /* If not, check the next item in case we're searching the list */
      node = node->next;
      if (node)
	{
	  new_long_name = list->long_namefunc(node->data);
	  if (strcmp(long_name, new_long_name) == 0)
	    {
	      set_long_name_cache(ulist, new_long_name, node);
	      return node;
	    }
	}
      /* If not, check the index cache */
      node = list->index_cache_node;
      if (node)
	{
	  new_long_name = list->long_namefunc(node->data);
	  if (strcmp(long_name, new_long_name) == 0)
	    {
	      set_long_name_cache(ulist, new_long_name, node);
	      return node;
	    }
	}
    }

  node = stp_list_get_item_by_long_name_internal(list, long_name);

  if (node)
    set_long_name_cache(ulist, long_name, node);

  return node;
}


/* callback for freeing data */
void
stp_list_set_freefunc(stp_list_t *list, stp_node_freefunc freefunc)
{
  check_list(list);
  list->freefunc = freefunc;
}

stp_node_freefunc
stp_list_get_freefunc(const stp_list_t *list)
{
  check_list(list);
  return list->freefunc;
}

/* callback for copying data */
void
stp_list_set_copyfunc(stp_list_t *list, stp_node_copyfunc copyfunc)
{
  check_list(list);
  list->copyfunc = copyfunc;
}

stp_node_copyfunc
stp_list_get_copyfunc(const stp_list_t *list)
{
  check_list(list);
  return list->copyfunc;
}

/* callback for getting data name */
void
stp_list_set_namefunc(stp_list_t *list, stp_node_namefunc namefunc)
{
  check_list(list);
  list->namefunc = namefunc;
}

stp_node_namefunc
stp_list_get_namefunc(const stp_list_t *list)
{
  check_list(list);
  return list->namefunc;
}

/* callback for getting data long_name */
void
stp_list_set_long_namefunc(stp_list_t *list, stp_node_namefunc long_namefunc)
{
  check_list(list);
  list->long_namefunc = long_namefunc;
}

stp_node_namefunc
stp_list_get_long_namefunc(const stp_list_t *list)
{
  check_list(list);
  return list->long_namefunc;
}

/* callback for sorting nodes */
void
stp_list_set_sortfunc(stp_list_t *list, stp_node_sortfunc sortfunc)
{
  check_list(list);
  list->sortfunc = sortfunc;
}

stp_node_sortfunc
stp_list_get_sortfunc(const stp_list_t *list)
{
  check_list(list);
  return list->sortfunc;
}


/* list item functions */

/* these functions operate on individual nodes in a list */

/*
 * create a new node in list, before next (may be null e.g. if sorting
 * next is calculated automatically, else defaults to end).  Must be
 * initialised with data (null nodes are disallowed).  The
 * stp_list_item_t type can not exist unless it is associated with an
 * stp_list_t list head.
 */
int
stp_list_item_create(stp_list_t *list,
		     stp_list_item_t *next,
		     const void *data)
{
  stp_list_item_t *ln; /* list node to add */
  stp_list_item_t *lnn; /* list node next */

  check_list(list);

  clear_cache(list);

  ln = stp_malloc(sizeof(stp_list_item_t));
  ln->prev = ln->next = NULL;

  if (data)
    ln->data = (void *) data;
  else
    {
      stp_free(ln);
      return 1;
    }

  if (list->sortfunc)
    {
      /* set np to the previous node (before the insertion */
      lnn = list->end;
      while (lnn)
	{
	  if (list->sortfunc(lnn->data, ln->data) <= 0)
	    break;
	  lnn = lnn->prev;
	}
    }
#if 0
  /*
   * This code #ifdef'ed out by Robert Krawitz on April 3, 2004.
   * Setting a debug variable should not result in taking a materially
   * different code path.
   */
  else if (stpi_get_debug_level() & STPI_DBG_LIST)
    {
      if (next)
	{
	  lnn = list->start;
	  while (lnn)
	    {
	      if (lnn == next)
		break;
	      lnn = lnn->prev;
	    }
	}
      else
	lnn = NULL;
    }
#endif
  else
    lnn = next;

  /* got lnp; now insert the new ln */

  /* set next */
  ln->next = lnn;

  if (!ln->prev) /* insert at start of list */
    {
      if (list->start) /* list not empty */
	ln->prev = list->end;
      else
	list->start = ln;
      list->end = ln;
    }

  /* set prev (already set if at start of list) */

  if (!ln->prev && ln->next) /* insert at end of list */
    ln->prev = ln->next->prev;

  if (list->start == ln->next) /* prev was old end */
    {
      list->start = ln;
    }

  /* set next->prev */
  if (ln->next)
    ln->next->prev = ln;

  /* set prev->next */
  if (ln->prev)
    ln->prev->next = ln;

  /* increment reference count */
  list->length++;

  stp_deprintf(STP_DBG_LIST, "stp_list_node constructor\n");
  return 0;
}

/* remove a node from list */
int
stp_list_item_destroy(stp_list_t *list, stp_list_item_t *item)
{
  check_list(list);

  clear_cache(list);
  /* decrement reference count */
  list->length--;

  if (list->freefunc)
    list->freefunc((void *) item->data);
  if (item->prev)
    item->prev->next = item->next;
  else
    list->start = item->next;
  if (item->next)
    item->next->prev = item->prev;
  else
    list->end = item->prev;
  stp_free(item);

  stp_deprintf(STP_DBG_LIST, "stp_list_node destructor\n");
  return 0;
}

/* get previous node */
stp_list_item_t *
stp_list_item_prev(const stp_list_item_t *item)
{
  return item->prev;
}

/* get next node */
stp_list_item_t *
stp_list_item_next(const stp_list_item_t *item)
{
  return item->next;
}

/* get data for node */
void *
stp_list_item_get_data(const stp_list_item_t *item)
{
  return item->data;
}

/* set data for node */
int
stp_list_item_set_data(stp_list_item_t *item, void *data)
{
  if (data)
    {
      item->data = data;
      return 0;
    }
  return 1; /* return error if data was NULL */
}
