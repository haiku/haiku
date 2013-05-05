#ifndef CACHE_H
#define CACHE_H
/* Cache - a template cache class
**
** Copyright 2001 pinc Software. All Rights Reserved.
*/


#include <SupportDefs.h>

#include <stdio.h>


template<class T> class Cache
{
	public:
		class Cacheable
		{
			public:
				Cacheable()
					:
					prev(NULL),
					next(NULL),
					locked(0L),
					isDirty(false)
				{
				}

				virtual ~Cacheable()
				{
					// perform your "undirty" work here
				}
				
				virtual bool Equals(T) = 0;

				Cacheable	*prev,*next;
				int32		locked;
				bool		isDirty;
		};
		
	public:
		Cache(int32 max = 20)
			:
			fMaxInQueue(max),
			fCount(0),
			fHoldChanges(false),
			fMostRecentlyUsed(NULL),
			fLeastRecentlyUsed(NULL)
		{
		}
		
		virtual ~Cache()
		{
			Clear(0,true);
		}

		void Clear(int32 targetCount = 0,bool force = false)
		{		
			Cacheable *entry = fLeastRecentlyUsed;
			while (entry)
			{
				Cacheable *prev = entry->prev;
				if (entry->locked <= 0 || force)
				{
					if (entry->next)
						entry->next->prev = prev;
					if (prev)
						prev->next = entry->next;

					if (fLeastRecentlyUsed == entry)
						fLeastRecentlyUsed = prev;
					if (fMostRecentlyUsed == entry)
						fMostRecentlyUsed = prev;

					delete entry;
					
					if (--fCount <= targetCount)
						break;
				}
		
				entry = prev;
			}
		}

		void SetHoldChanges(bool hold)
		{
			fHoldChanges = hold;
			if (!hold)
				Clear(fMaxInQueue);
		}
		
		status_t SetDirty(T data,bool dirty)
		{
			Cacheable *entry = Get(data);
			if (!entry)
				return B_ENTRY_NOT_FOUND;

			entry->isDirty = dirty;
			return B_OK;
		}
		
		status_t Lock(T data)
		{
			Cacheable *entry = Get(data);
			if (!entry)
				return B_ENTRY_NOT_FOUND;

			entry->locked++;
			return B_OK;
		}
		
		status_t Unlock(T data)
		{
			Cacheable *entry = Get(data);
			if (!entry)
				return B_ENTRY_NOT_FOUND;

			entry->locked--;
			return B_OK;
		}
		
		Cacheable *Get(T data)
		{
			Cacheable *entry = GetFromCache(data);
			if (entry)
			{
				if (fMostRecentlyUsed == entry)
					return entry;

				// remove entry from cache (to insert it at top of the MRU list)
				if (entry->prev)
					entry->prev->next = entry->next;
				if (!entry->next)
				{
					if (fLeastRecentlyUsed == entry)
						fLeastRecentlyUsed = entry->prev;
					else
						fprintf(stderr,"Cache: fatal error, fLeastRecentlyUsed != entry\n");
				}
				else
					entry->next->prev = entry->prev;
			}
			else
			{
				entry = NewCacheable(data);
				if (entry)
					fCount++;
			}

			if (entry)
			{
				// insert entry at the top of the MRU list
				entry->next = fMostRecentlyUsed;
				entry->prev = NULL;

				if (fMostRecentlyUsed)
					fMostRecentlyUsed->prev = entry;
				else if (fLeastRecentlyUsed == NULL)
					fLeastRecentlyUsed = entry;
				else
					fprintf(stderr,"Cache: fatal error, fLeastRecently != NULL!\n");
				fMostRecentlyUsed = entry;
				
				// remove old nodes from of the cache (if possible and necessary)
				if (!fHoldChanges
					&& fCount > fMaxInQueue
					&& fLeastRecentlyUsed)
					Clear(fMaxInQueue);
			}
			return entry;
		}
		
	protected:
		Cacheable *GetFromCache(T data)
		{
			Cacheable *entry = fMostRecentlyUsed;
			while (entry)
			{
				if (entry->Equals(data))
					return entry;
				entry = entry->next;
			}
			return NULL;
		}
		
		virtual Cacheable *NewCacheable(T data) = 0;

		int32		fMaxInQueue, fCount;
		bool		fHoldChanges;
		Cacheable	*fMostRecentlyUsed;
		Cacheable	*fLeastRecentlyUsed;
};

#endif	/* CACHE_H */
