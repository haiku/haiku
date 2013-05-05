/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef UTIL_H
#define UTIL_H

#include <memory.h>
#include <stdlib.h>
#include "error.h"

template <class contents>
struct LispNode {
	contents* car;
	LispNode* cdr;

	/* Create a node with no next */
	inline LispNode(contents* value)
		: car(value), cdr(0) { }

	/* Create a node with specified next */
	inline LispNode(contents* value, LispNode* next)
 		: car (value), cdr(next) { }

	/* Create a node and insert it in a list right after `prev' */
	inline LispNode(LispNode* prev, contents* value)
		: car(value), cdr(prev->cdr) { prev->cdr = this; }
};


template <class contents>
struct LispList {
	LispNode<contents> *first;

	/* -------- List creation --------------- */
	/* Create an empty list */
	inline LispList()
	{
		first = 0;
	}


	/* Create a list pointing to the specified node */
	inline LispList(LispNode<contents>* _first)
	{
		first = _first;
	}


	/* ?? */
	inline LispList(LispList &init)
	{
		first = init.first;
	}


	/* ---------- List queries ------------- */
	inline int is_empty()
	{
		return first == 0;
	}


	/* Determines if a thing is on the list */
	inline int is_present(contents* element)
	{
		for (LispNode<contents>* node = first; node; node = node->cdr)
			if (node->car == element)
				return 1;
		return 0;
	}


	/* Returns the length of the list */
	inline int count()
	{
		int n = 0;
		for (LispNode<contents>* node = first; node; node = node->cdr)
			n++;
	    return n;
	}


	/* ----------- Adding "nodes" to the list ------------ */
	/* Add a specified node to the head of the list.  */
	inline void add_head(LispNode<contents>* new_element)
	{
		new_element->cdr = first;
		first = new_element;
	}


	/* Add a specified node anywhere on the list */
	inline void add(LispNode<contents>* new_element)
	{
		add_head (new_element);
	}


	inline void add_tail(LispNode<contents>* new_element)
	{
		LispNode<contents>** pred = &first;
		while(*pred)
			pred = &(*pred)->cdr;
		*pred = new_element;
		new_element->cdr = 0;
	}


	/* ----------- Adding "contents" to the list ------------ */
	/* -----  (Which in my opinion is far more useful) ------ */

	/* Create new node pointing to thing, & add to head of list. */
	inline void add_head_new(contents* new_element)
	{
		first = new LispNode<contents>(new_element, first);
	}


	inline void add_head(contents* new_element)
	{
		add_head_new(new_element);
	}


	/* Create new node pointing to thing, & add to end of list. */
	inline void add_tail_new(contents* new_element)
	{
		LispNode< contents >** pred = &first;
		while (*pred)
			pred = &(*pred)->cdr;
			
		*pred = new LispNode< contents >(new_element);
	}
	

	inline void add_tail(contents* new_element)
	{
		add_tail_new(new_element);
	}


	/* Create new node pointing to thing, & add anywhere on list */
	inline void add_new(contents* new_element)
	{
		add_head_new(new_element);
	}


	inline void add(contents* new_element)
	{
		add_new(new_element);
	}

	
	/* Create and add a new node for a specified element, 
		but only if it's not already on the list.  */
	inline void add_new_once(contents* new_element)
	{
		if (!is_present(new_element))
			add_head_new(new_element);
	}
	
	
	inline void add_tail_once(contents *new_element)
	{
		if (!is_present(new_element))
			add_tail_new(new_element);
	}

  
	/* ------- Removing things from the list ------------ */

	/* Remove and return the first node on the list j.h. */
	inline LispNode<contents>* rem_head ()
	{
		LispNode<contents>* n = first;
		
		if (n) {
			first = first->cdr;
		}
		
		return n;
	}

	
	/* Remove and return any node on the list.  */
	inline LispNode<contents>* remove ()
	{
		return( rem_head() ); 
	}

	
	/* Remove a specified node from the list.  */
	inline void remove (LispNode<contents>* node)
	{
		for (LispNode<contents> **pp = &first; *pp; pp = &(*pp)->cdr)
			if (*pp == node) {
				*pp = (*pp)->cdr;
		  		return;
			}
	}


	/* Remove & delete all nodes pointing to a particular element. */
	inline void rem_del (contents* element)
	{
		LispNode<contents>** pp = &first;
		while (*pp) { 
			if ((*pp)->car == element) {
				LispNode<contents> *o = *pp;
				*pp = o->cdr;
				delete o;
			} else
				pp = &(*pp)->cdr;
		}
	}


	/* Remove and delete all nodes on the list.  */
	inline void rem_del_all()
	{
		while (first) {
			LispNode<contents>* old = first;
			first = first->cdr;
			delete old;
		}
	}


	/* -------- Simple list storage (by j.h.) ------------ */

	/* When you just want to hold a bunch of stuff on a list and then pull them 
		off later.  Note that these calls do NOT check for to see if a thing is 
		already on the list.  Use is_present() before adding.
	*/
	/* Put something anywhere on the list */
	inline void put( contents* c )
	{
		add_tail( c );
	}


	/* Put something at beginning of list */
	inline void put_head( contents* c )
	{
		add_head( c );
	}


	/* Put something at end of the list */
	inline void put_tail( contents* c )
	{
		add_tail( c );
	}

#if 0 /* leaks memory */
	  /* Take a specific thing off the list */
	  inline contents *get(contents *element)
	    {
	      contents *c = 0;
	      for (LispNode<contents> *node = first; node; node = node->cdr)
	      {
		if (node->car == element)
		{
		  c = node->car;
		  remove(node);
		  break;
		}
	      }
	      return c;
	    }
#endif

	/* Take the first thing off the list */
	inline contents* get_head()
	{
		contents *c = 0;
		if(first) {
			c = first->car;
			delete rem_head();
		}
		return c;
	}
	
	
	/* Take something off the list */
	inline contents* get()
	{
		return(get_head());
	}
	

	/* XXX inline contents *get_tail() { } */

/* -------- Stack simulation (by j.h.) ------------ */
	/* Put a thing onto the head of the list */
	inline void push(contents* c)
	{
		put_head(c);
	}


	/* Remove a thing from the head of the list */
	inline contents* pop()
	{
		return(get_head());
	}

	/* Pop everything off the stack.  Empty the stack/list. */
	inline void pop_all()
	{
		rem_del_all();
	}


	/* ----------- list/list manipulations ------------ */

	/* Add all elements present on another list to this list also. */
	inline void add_new(LispList other)
	{
		for (LispNode<contents>* n = other.first; n; n = n->cdr)
			add_new(n->car);
	}
	
	
	inline void add_new_once(LispList other)
	{
		for (LispNode<contents>* n = other.first; n; n = n->cdr)
			add_new_once(n->car);
	}
	

	/* Remove and delete all nodes whose contents are also present
	   in a different list (set disjunction). */
	inline void rem_del(LispList other)
	{
		for (LispNode<contents>* n = other.first; n; n = n->cdr)
			rem_del(n->car);
	}
};

template <class thetype>
struct DoubleLinkedNode
{
	thetype* next;
	thetype* prev;

	DoubleLinkedNode()
	{
		next = prev = NULL;
	}
 
 
	void insert_after(thetype* n)
	{
		if (next != NULL)
			next->prev = n;
			
		n->next = next;
		n->prev = (thetype*)this;
		next = n;
	}


	void insert_before(thetype* n)
	{
		prev->next = n;
		n->next = (thetype*)this;
		n->prev = prev;
		prev = n;
	}


	void remove()
	{
		assert(prev != NULL);
		prev->next = next;
		
		if (next != NULL)
			next->prev = prev;
	}
};


template <class thetype>
struct DoubleLinkedList : public DoubleLinkedNode<thetype>
{
	DoubleLinkedList() : DoubleLinkedNode<thetype>() {};

	void insert(thetype* n)
	{
		insert_after(n);
	}
	

	void add(thetype* n)
	{
		insert_after(n);
	}
};


template <class T>
struct BufferArray {
	T * items;
	int num_items;
	int num_slots;
	int slot_inc;

	void resize(int i)
	{
		items = (T*)realloc(items,sizeof(T)*i);
		num_slots = i;
	}
	

	T & operator [](int index)
	{
		assert(index < num_items);
		return items[index];
	}
	

	T & get(int index)
	{
		assert(index < num_items);
		return items[index];
	}
	

	void add(T &item)
	{
		if (num_items == num_slots)
			resize(num_slots+slot_inc);
	
		memcpy(items+num_items,&item,sizeof(item));
		num_items++;
	}


	BufferArray(int start_slots, int _slot_inc)
	{
		num_slots = start_slots;
		slot_inc = _slot_inc;
		assert(slot_inc > 0);
		num_items = 0;
		items = (T*)malloc(sizeof(T)*num_slots);
	}


	~BufferArray()
	{
		free(items);
	}
};

#endif // UTIL_H
