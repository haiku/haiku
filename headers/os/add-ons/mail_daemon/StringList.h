#ifndef ZOIDBERG_STRING_LIST_H
#define ZOIDBERG_STRING_LIST_H
/* StringList - a string list implementation
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Flattenable.h>

class BList;

class BStringList : public BFlattenable {
	public:
		BStringList();
		BStringList(const BStringList&);
		
		~BStringList(void);
		
		BStringList	&operator=(const BStringList &from);
		
/* Flattenable stuff */
		virtual	bool		IsFixedSize() const; //--false for obvious reasons
		virtual	type_code	TypeCode() const;
		virtual	ssize_t		FlattenedSize() const;
		virtual	status_t	Flatten(void *buffer, ssize_t size) const;
		virtual	bool		AllowsTypeCode(type_code code) const;
		virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

/* Adding and removing items. */		
		void	AddItem(const char *item);
		void	AddList(const BStringList *newItems);
		bool	RemoveItem(const char *item);
		void	MakeEmpty();

/* Retrieving items. */
		const char *ItemAt(int32) const;
		int32 IndexOf(const char *) const;

/* Querying the list. */
		bool	HasItem(const char *item) const;
		int32	CountItems() const;
		bool	IsEmpty() const;

/* Determining differences between lists */
		void NotHere(BStringList &other_list, BStringList *results);
		void NotThere(BStringList &other_list, BStringList *results);
		
/* Useful list logic operators */
		BStringList	&operator += (const char *item);
		BStringList	&operator += (BStringList &list);
		
		BStringList	&operator -= (const char *item);
		BStringList	&operator -= (BStringList &list);
		
		BStringList	operator | (BStringList &list);
		BStringList	&operator |= (BStringList &list);
		
		BStringList	operator ^ (BStringList &list);
		BStringList	&operator ^= (BStringList &list);
		
		bool		operator == (BStringList &list);
		const char *operator [] (int32 index);
		
	private:
		void *_buckets[256];
		int32 _items;
		BList *_indexed;
		
		uint32 _reserved[5];
};

#endif	/* ZOIDBERG_STRING_LIST_H */
