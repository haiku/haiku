/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/*
	Macros for double linked lists
*/

#ifndef _DL_LIST_H
#define _DL_LIST_H

#define REMOVE_DL_LIST( item, head, prefix ) \
	do { \
		if( item->prefix##prev ) \
			item->prefix##prev->prefix##next = item->prefix##next; \
		else \
			head = item->prefix##next; \
\
		if( item->prefix##next ) \
			item->prefix##next->prefix##prev = item->prefix##prev; \
	} while( 0 )
		
#define ADD_DL_LIST_HEAD( item, head, prefix ) \
	do { \
		item->prefix##next = head; \
		item->prefix##prev = NULL; \
\
		if( (head) ) \
			(head)->prefix##prev = item; \
\
		(head) = item; \
	} while( 0 )

#define REMOVE_CDL_LIST( item, head, prefix ) \
	do { \
		item->prefix##next->prefix##prev = item->prefix##prev; \
		item->prefix##prev->prefix##next = item->prefix##next; \
 \
		if( item == (head) ) { \
			if( item->prefix##next != item ) \
				(head) = item->prefix##next; \
			else \
				(head) = NULL; \
		} \
	} while( 0 )

#define ADD_CDL_LIST_TAIL( item, type, head, prefix ) \
	do { \
		type *old_head = head; \
 \
		if( old_head ) { \
			type *first, *last; \
 \
			first = old_head; \
			last = first->prefix##prev; \
 \
			item->prefix##next = first; \
			item->prefix##prev = last; \
			first->prefix##prev = item; \
			last->prefix##next = item; \
		} else { \
			head = item; \
			item->prefix##next = item->prefix##prev = item; \
		} \
	} while( 0 )

#define ADD_CDL_LIST_HEAD( item, type, head, prefix ) \
	do { \
		type *old_head = head; \
 \
 		head = item; \
		if( old_head ) { \
			type *first, *last; \
 \
			first = old_head; \
			last = first->prefix##prev; \
 \
			item->prefix##next = first; \
			item->prefix##prev = last; \
			first->prefix##prev = item; \
			last->prefix##next = item; \
		} else { \
			item->prefix##next = item->prefix##prev = item; \
		} \
	} while( 0 )

#endif
