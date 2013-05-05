/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef DIRECTORYCACHE_H
#define DIRECTORYCACHE_H


#include <lock.h>
#include <SupportDefs.h>
#include <util/DoublyLinkedList.h>
#include <util/KernelReferenceable.h>
#include <util/SinglyLinkedList.h>


class Inode;

struct NameCacheEntry :
	public SinglyLinkedListLinkImpl<NameCacheEntry> {
			ino_t			fNode;
			const char*		fName;

							NameCacheEntry(const char* name, ino_t node);
							NameCacheEntry(const NameCacheEntry& entry);
							~NameCacheEntry();
};

struct DirectoryCacheSnapshot : public KernelReferenceable {
			SinglyLinkedList<NameCacheEntry>	fEntries;
	mutable	mutex			fLock;

							DirectoryCacheSnapshot();
							DirectoryCacheSnapshot(
								const DirectoryCacheSnapshot& snapshot);
							~DirectoryCacheSnapshot();
};

class DirectoryCache {
public:
							DirectoryCache(Inode* inode, bool attr = false);
							~DirectoryCache();

	inline	void			Lock();
	inline	void			Unlock();

			void			Reset();
			void			Trash();
	inline	bool			Valid();

			status_t		AddEntry(const char* name, ino_t node,
								bool created = false);
			void			RemoveEntry(const char* name);

	inline	status_t		GetSnapshot(DirectoryCacheSnapshot** snapshot);

	inline	SinglyLinkedList<NameCacheEntry>&	EntriesList();

			status_t		Revalidate();

	inline  status_t		ValidateChangeInfo(uint64 change);
	inline  void			SetChangeInfo(uint64 change);
	inline	uint64			ChangeInfo();

	inline	Inode*			GetInode();

	const	bigtime_t		fExpirationTime;
protected:
			void			NotifyChanges(DirectoryCacheSnapshot* oldSnapshot,
								DirectoryCacheSnapshot* newSnapshot);

private:
			void			_SetSnapshot(DirectoryCacheSnapshot* snapshot);
			status_t		_LoadSnapshot(bool trash);

			SinglyLinkedList<NameCacheEntry>	fNameCache;

			DirectoryCacheSnapshot*	fDirectoryCache;

			Inode*			fInode;

			bool			fAttrDir;
			bool			fTrashed;
			mutex			fLock;

			uint64			fChange;
			bigtime_t		fExpireTime;
};


inline void
DirectoryCache::Lock()
{
	mutex_lock(&fLock);
}


inline void
DirectoryCache::Unlock()
{
	mutex_unlock(&fLock);
}


inline bool
DirectoryCache::Valid()
{
	return !fTrashed;
}


inline status_t
DirectoryCache::GetSnapshot(DirectoryCacheSnapshot** snapshot)
{
	ASSERT(snapshot != NULL);

	status_t result = B_OK;
	if (fDirectoryCache == NULL)
		result = _LoadSnapshot(false);
	*snapshot = fDirectoryCache;
	return result;
}


inline SinglyLinkedList<NameCacheEntry>&
DirectoryCache::EntriesList()
{
	return fNameCache;
}


inline status_t
DirectoryCache::ValidateChangeInfo(uint64 change)
{
	if (fTrashed || change != fChange) {
		Trash();
		fChange = change;
		fExpireTime = system_time() + fExpirationTime;
		fTrashed = false;

		return B_ERROR;
	}

	return B_OK;
}


inline void
DirectoryCache::SetChangeInfo(uint64 change)
{
	fExpireTime = system_time() + fExpirationTime;
	fChange = change;
}


inline uint64
DirectoryCache::ChangeInfo()
{
	return fChange;
}


inline Inode*
DirectoryCache::GetInode()
{
	return fInode;
}


#endif	// DIRECTORYCACHE_H
			
