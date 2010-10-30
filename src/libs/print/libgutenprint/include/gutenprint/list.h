/*
 * "$Id: list.h,v 1.2 2004/11/28 15:59:29 rleigh Exp $"
 *
 *   libgimpprint generic list type
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
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

/**
 * @file gutenprint/list.h
 * @brief Generic list functions.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_LIST_H
#define GUTENPRINT_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The list data type implements a fast generic doubly-linked list.
 * It supports all of the operations you might want in a list (insert,
 * remove, iterate over the list, copy whole lists), plus some
 * (optional) less common features: finding items by index, name or
 * long name, and sorting.  These should also be fairly fast, due to
 * caching in the list head.
 *
 * @defgroup list list
 * @{
 */

  struct stp_list_item;
  /**
   * The list item opaque data type.
   * This object is a node in the list.
   */
  typedef struct stp_list_item stp_list_item_t;

  struct stp_list;
  /**
   * The list opaque data type.
   * This object represents the list as a whole.
   */
  typedef struct stp_list stp_list_t;

  /**
   * A callback function to free the data a node contains.
   * The parameter is a pointer to the node data.
   */
  typedef void (*stp_node_freefunc)(void *);

  /**
   * A callback function to copy the data a node contains.
   * The parameter is a pointer to the node data.
   * The return value is a pointer to the new copy of the data.
   */
  typedef void *(*stp_node_copyfunc)(const void *);

  /**
   * A callback function to get the name of a node.
   * The parameter is a pointer to the node data.
   * The return value is a pointer to the name of the
   * node, or NULL if there is no name.
   */
  typedef const char *(*stp_node_namefunc)(const void *);

  /**
   * A callback function to compare two nodes.
   * The two parameters are pointers to node data.
   * The return value is <0 if the first sorts before the
   * second, 0 if they sort identically, and >0 if the
   * first sorts after the second.
   */
  typedef int (*stp_node_sortfunc)(const void *, const void *);

  /**
   * Free node data allocated with stp_malloc.
   * This function is indended for use as an stp_node_freefunc, which
   * uses stp_free to free the node data.
   * @param item the node data to free
   */
  extern void stp_list_node_free_data(void *item);

  /**
   * Create a new list object.
   * @returns the newly created list object.
   */
  extern stp_list_t *stp_list_create(void);

  /**
   * Copy and allocate a list object.
   * list must be a valid list object previously created with
   * stp_list_create().
   * @param list the list to copy.
   * @returns a pointer to the new copy of the list.
   */
  extern stp_list_t *stp_list_copy(const stp_list_t *list);

  /**
   * Destroy a list object.
   * It is an error to destroy the list more than once.
   * @param list the list to destroy.
   * @returns 0 on success, 1 on failure.
   */
  extern int stp_list_destroy(stp_list_t *list);

  /**
   * Find the first item in a list.
   * @param list the list to use.
   * @returns a pointer to the first list item, or NULL if the list is
   * empty.
   */
  extern stp_list_item_t *stp_list_get_start(const stp_list_t *list);

  /**
   * Find the last item in a list.
   * @param list the list to use.
   * @returns a pointer to the last list item, or NULL if the list is
   * empty.
   */
  extern stp_list_item_t *stp_list_get_end(const stp_list_t *list);

  /**
   * Find an item in a list by its index.
   * @param list the list to use.
   * @param idx the index to find.
   * @returns a pointer to the list item, or NULL if the index is
   * invalid or the list is empty.
   */
  extern stp_list_item_t *stp_list_get_item_by_index(const stp_list_t *list,
						     int idx);

  /**
   * Find an item in a list by its name.
   * @param list the list to use.
   * @param name the name to find.
   * @returns a pointer to the list item, or NULL if the name is
   * invalid or the list is empty.
   */
  extern stp_list_item_t *stp_list_get_item_by_name(const stp_list_t *list,
						    const char *name);

  /**
   * Find an item in a list by its long name.
   * @param list the list to use.
   * @param long_name the long name to find.
   * @returns a pointer to the list item, or NULL if the long name is
   * invalid or the list is empty.
   */
  extern stp_list_item_t *stp_list_get_item_by_long_name(const stp_list_t *list,
							 const char *long_name);

  /**
   * Get the length of a list.
   * @param list the list to use.
   * @returns the list length (number of list items).
   */
  extern int stp_list_get_length(const stp_list_t *list);

  /**
   * Set a list node free function.
   * This callback function will be called whenever a list item is
   * destroyed.  Its intended use is for automatic object destruction
   * and any other cleanup required.
   * @param list the list to use.
   * @param freefunc the function to set.
   */
  extern void stp_list_set_freefunc(stp_list_t *list,
				    stp_node_freefunc freefunc);

  /**
   * Get a list node free function.
   * @param list the list to use.
   * @returns the function previously set with stp_list_set_freefunc,
   * or NULL if no function has been set.
   */
  extern stp_node_freefunc stp_list_get_freefunc(const stp_list_t *list);

  /**
   * Set a list node copy function.
   * This callback function will be called whenever a list item is
   * copied.  Its intended use is for automatic object copying
   * (since C lacks a copy constructor).
   * @param list the list to use.
   * @param copyfunc the function to set.
   */
  extern void stp_list_set_copyfunc(stp_list_t *list,
				    stp_node_copyfunc copyfunc);

  /**
   * Get a list node copy function.
   * @param list the list to use.
   * @returns the function previously set with stp_list_set_copyfunc,
   * or NULL if no function has been set.
   */
  extern stp_node_copyfunc stp_list_get_copyfunc(const stp_list_t *list);

  /**
   * Set a list node name function.
   * This callback function will be called whenever the name of a list
   * item needs to be determined.  This is used to find list items by
   * name.
   * @param list the list to use.
   * @param namefunc the function to set.
   */
  extern void stp_list_set_namefunc(stp_list_t *list,
				    stp_node_namefunc namefunc);

  /**
   * Get a list node name function.
   * @param list the list to use.
   * @returns the function previously set with stp_list_set_namefunc,
   * or NULL if no function has been set.
   */
  extern stp_node_namefunc stp_list_get_namefunc(const stp_list_t *list);

  /**
   * Set a list node long name function.
   * This callback function will be called whenever the long name of a list
   * item needs to be determined.  This is used to find list items by
   * long name.
   * @param list the list to use.
   * @param long_namefunc the function to set.
   */
  extern void stp_list_set_long_namefunc(stp_list_t *list,
					 stp_node_namefunc long_namefunc);

  /**
   * Get a list node long name function.
   * @param list the list to use.
   * @returns the function previously set with
   * stp_list_set_long_namefunc, or NULL if no function has been set.
   */
  extern stp_node_namefunc stp_list_get_long_namefunc(const stp_list_t *list);

  /**
   * Set a list node sort function.
   * This callback function will be called to determine the sort
   * order for list items in sorted lists.
   * @param list the list to use.
   * @param sortfunc the function to set.
   */
  extern void stp_list_set_sortfunc(stp_list_t *list,
				    stp_node_sortfunc sortfunc);

  /**
   * Get a list node sort function.
   * @param list the list to use.
   * @returns the function previously set with
   * stp_list_set_sortfunc, or NULL if no function has been set.
   */
  extern stp_node_sortfunc stp_list_get_sortfunc(const stp_list_t *list);

  /**
   * Create a new list item.
   * @param list the list to use.
   * @param next the next item in the list, or NULL to insert at the end of
   * the list.
   * @param data the data the list item will contain.
   * @returns 0 on success, 1 on failure (if data is NULL, for example).
   */
  extern int stp_list_item_create(stp_list_t *list,
				  stp_list_item_t *next,
				  const void *data);

  /**
   * Destroy a list item.
   * @param list the list to use.
   * @param item the item to destroy.
   * @returns 0 on success, 1 on failure.
   */
  extern int stp_list_item_destroy(stp_list_t *list,
				   stp_list_item_t *item);

  /**
   * Get the previous item in the list.
   * @param item the item to start from.
   * @returns a pointer to the list item prior to item, or NULL if
   * item is the start of the list.
   */
  extern stp_list_item_t *stp_list_item_prev(const stp_list_item_t *item);

  /**
   * Get the next item in the list.
   * @param item the item to start from.
   * @returns a pointer to the list item following from item, or NULL
   * if item is the end of the list.
   */
  extern stp_list_item_t *stp_list_item_next(const stp_list_item_t *item);

  /**
   * Get the data associated with a list item.
   * @param item the list item to use.
   * @returns the data associated with item.
   */
  extern void *stp_list_item_get_data(const stp_list_item_t *item);

  /**
   * Set the data associated with a list item.
   * @warning Note that if a sortfunc is in use, changing the data
   * will NOT re-sort the list!
   * @param item the list item to use.
   * @param data the data to set.
   * @returns 0 on success, 1 on failure (if data is NULL).
   */
  extern int stp_list_item_set_data(stp_list_item_t *item,
				    void *data);

  /** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_LIST_H */
