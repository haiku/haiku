#ifndef _MEDIA_T_MAP_H
#define _MEDIA_T_MAP_H


#include "MediaDebug.h"

template<class key, class value> class Map
{
public:
	Map()
	 :	item_max(INIT_COUNT),
	 	item_count(0),
	 	item_iter(-1),
	 	items((ent **)malloc(sizeof(ent *) * INIT_COUNT))
	{
		ASSERT(items);
	}
	
	~Map()
	{
		MakeEmpty();
		free(items);
	}

	Map(const Map<key, value> &other)
	{
		*this = other;
	}

	Map<key, value> &operator=(const Map<key, value> &other)
	{
		MakeEmpty();
		free(items);
		item_max = other.item_max;
	 	item_count = other.item_count;
		items = (ent **)malloc(sizeof(ent *) * item_max);
		ASSERT(items);
	 	for (int i = 0; i < item_count; i++) {
	 		items[i] = new ent;
	 		items[i]->k = other.items[i]->k;
	 		items[i]->v = other.items[i]->v;
	 	}
	 	return *this;
	}
		
	bool Insert(const key &k, const value &v)	
	{
		if (item_count == item_max) {
			item_max *= 2;
			items = (ent **)realloc(items, sizeof(ent *) * item_max);
			ASSERT(items);
		}
		items[item_count] = new ent;
		items[item_count]->k = k;
		items[item_count]->v = v;
		item_count++;
		return true;
	}
	
	bool Get(const key &k, value **v) const
	{
	 	for (int i = 0; i < item_count; i++) {
	 		if (items[i]->k == k) {
	 			*v = &items[i]->v;
	 			return true;
	 		}
	 	}
		return false;
	}

	bool Remove(const key &k) {
		for (int i = 0; i < item_count; i++)
			if (items[i]->k == k)
				return _Remove(i);
		return false;
	}
	
	int Find(const value &v) const
	{
		for (int i = 0; i < item_count; i++)
			if (items[i]->v == v)
				return i;
		return -1;
	}

	bool Has(const key &k) const
	{
		for (int i = 0; i < item_count; i++)
			if (items[i]->k == k)
				return true;
		return false;
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
		return _Get(item_iter, v);
	}
	
	bool GetCurrentKey(key **k)
	{
		if (item_iter < 0 || item_iter >= item_count)
			return false;
		*k = &items[item_iter]->k;
		return true;
	}
	
	bool RemoveCurrent()
	{
		return _Remove(item_iter);
	}

private:	
	bool _Get(int32 index, value **v) const
	{
		if (index < 0 || index >= item_count)
			return false;
		*v = &items[index]->v;
		return true;
	}
	
	bool _Remove(int32 index) 
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

private:
	enum { INIT_COUNT=32 };
	int item_max;
	int item_count;
	int item_iter;
	struct ent {
		key k;
		value v;
	};
	ent **items;
};

#endif // _MEDIA_T_MAP_H
