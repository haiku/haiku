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
							~NameCacheEntry();
};

struct DirectoryCacheSnapshot : public KernelReferenceable {
	SinglyLinkedList<NameCacheEntry>	fEntries;
	mutex								fLock;

										DirectoryCacheSnapshot();
										~DirectoryCacheSnapshot();
};

class DirectoryCache : public DoublyLinkedListLinkImpl<DirectoryCache> {
public:
							DirectoryCache(Inode* inode, bool attr = false);
							~DirectoryCache();

	inline	status_t		Lock();
	inline	void			Unlock();

			void			ResetAndLock();
			void			Trash();

			status_t		AddEntry(const char* name, ino_t node,
								bool created = false);
			void			RemoveEntry(const char* name);

			void			SetSnapshot(DirectoryCacheSnapshot* snapshot);
	inline	DirectoryCacheSnapshot* GetSnapshot();

	inline	SinglyLinkedList<NameCacheEntry>&	EntriesList();

			status_t		Revalidate();

	inline  status_t		ValidateChangeInfo(uint64 change);
	inline  void			SetChangeInfo(uint64 change);
	inline	uint64			ChangeInfo();

	inline	Inode*			GetInode();
	inline	time_t			ExpireTime();

	static	const bigtime_t	kExpirationTime		= 15000000;

			bool			fRevalidated;
protected:
			void			NotifyChanges(DirectoryCacheSnapshot* oldSnapshot,
								DirectoryCacheSnapshot* newSnapshot);

private:
			SinglyLinkedList<NameCacheEntry>	fNameCache;

			DirectoryCacheSnapshot*	fDirectoryCache;

			Inode*			fInode;

			bool			fAttrDir;
			bool			fTrashed;
			mutex			fLock;

			uint64			fChange;
			bigtime_t		fExpireTime;
};


inline status_t
DirectoryCache::Lock()
{
	mutex_lock(&fLock);
	if (fTrashed) {
		mutex_unlock(&fLock);
		return B_ERROR;
	}

	return B_OK;
}

inline void
DirectoryCache::Unlock()
{
	mutex_unlock(&fLock);
}


inline DirectoryCacheSnapshot*
DirectoryCache::GetSnapshot()
{
	return fDirectoryCache;
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
		fExpireTime = system_time() + kExpirationTime;
		fTrashed = false;

		return B_ERROR;
	}

	return B_OK;
}


inline void
DirectoryCache::SetChangeInfo(uint64 change)
{
	fExpireTime = system_time() + kExpirationTime;
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


inline time_t
DirectoryCache::ExpireTime()
{
	return fExpireTime;
}


#endif	// DIRECTORYCACHE_H
			
