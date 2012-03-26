#ifndef HASHTABLE_H
#define HASHTABLE_H
/* Hashtable - a general purpose hash table
**
** Copyright 2001 pinc Software. All Rights Reserved.
*/


#include "SupportDefs.h"


#define HASH_EMPTY_NONE 0
#define HASH_EMPTY_FREE 1
#define HASH_EMPTY_DELETE 2

class Hashtable
{
	public:
		Hashtable(int capacity = 100, float loadFactor = 0.75);
		~Hashtable();

		void	SetHashFunction(uint32 (*func)(const void *));
		void	SetCompareFunction(bool (*func)(const void *, const void *));

		bool	IsEmpty() const;
		bool	ContainsKey(const void *key);
		void	*GetValue(const void *key);

		bool	Put(const void *key, void *value);
		void	*Remove(const void *key);

		status_t GetNextEntry(void **value);
		void	Rewind();

		void	MakeEmpty(int8 keyMode = HASH_EMPTY_NONE,int8 valueMode = HASH_EMPTY_NONE);
		
	protected:
		class Entry
		{
			public:
				Entry(Entry *_next, const void *_key, void *_value)
					: next(_next), key(_key), value(_value) {}

				Entry		*next;
				const void	*key;
				void		*value;
		};

		bool	Rehash();
		Entry	*GetHashEntry(const void *key);

		int32	fCapacity,fCount,fThreshold,fModCount;
		float	fLoadFactor;
		Entry	**fTable;
		uint32	(*fHashFunc)(const void *);
		bool	(*fCompareFunc)(const void *, const void *);

		int32	fIteratorIndex;
		Entry	*fIteratorEntry;
};

#endif  // HASHTABLE_H
