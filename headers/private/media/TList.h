#ifndef _MEDIA_T_LIST_H
#define _MEDIA_T_LIST_H

#include "MediaDebug.h"

template<class value> class List
{
public:
	List()
	 :	item_max(INIT_COUNT),
	 	item_count(0),
	 	item_iter(-1),
	 	items((value **)malloc(sizeof(value *) * INIT_COUNT))
	{
		ASSERT(items);
	}
	
	~List()
	{
		MakeEmpty();
		free(items);
	}

	List(const List<value> &other)
	{
		*this = other;
	}

	List<value> &operator=(const List<value> &other)
	{
		MakeEmpty();
		free(items);
		item_max = other.item_max;
	 	item_count = other.item_count;
		items = (value **)malloc(sizeof(value *) * item_max);
		ASSERT(items);
	 	for (int i = 0; i < item_count; i++) {
	 		items[i] = new value;
	 		*items[i] = *other.items[i];
	 	}
	 	return *this;
	}
		
	bool Insert(const value &v)
	{
		if (item_count == item_max) {
			item_max *= 2;
			items = (value **)realloc(items, sizeof(value *) * item_max);
			ASSERT(items);
		}
		items[item_count] = new value;
		*items[item_count] = v;
		item_count++;
		return true;
	}
	
	bool Get(int32 index, value **v) const
	{
		if (index < 0 || index >= item_count)
			return false;
		*v = items[index];
		return true;
	}
	
	bool Remove(int32 index) 
	{
		if (index < 0 || index >= item_count)
			return false;
		delete items[index];
		item_count--;
		items[index] = items[item_count];
		if (index == item_iter)
			item_iter--;
		return true;
	}
	
	int Find(const value &v) const
	{
		for (int i = 0; i < item_count; i++)
			if (*items[i] == v)
				return i;
		return -1;
	}
	
	int CountItems() const
	{
		return item_count;
	}
	
	bool IsEmpty() const
	{
		return item_count == 0;
	}
	
	void MakeEmpty()
	{
		if (items != 0) {
			for (int i = 0; i < item_count; i++) {
				delete items[i];
			}
			item_count = 0;
		}
	}
	
	void Rewind()
	{
		item_iter = -1;
	}

	bool GetNext(value **v)
	{
		item_iter++;
		return Get(item_iter, v);
	}
	
	bool RemoveCurrent()
	{
		return Remove(item_iter);
	}

private:
	enum { INIT_COUNT=32 };
	int item_max;
	int item_count;
	int item_iter;
	value **items;
};

#endif // _MEDIA_T_LIST_H
