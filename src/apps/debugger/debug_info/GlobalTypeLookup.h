/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GLOBAL_TYPE_LOOKUP_H
#define GLOBAL_TYPE_LOOKUP_H


#include <image.h>
#include <Locker.h>

#include <Referenceable.h>
#include <util/OpenHashTable.h>


class BString;
class Type;


class GlobalTypeCache : public Referenceable {
public:
								GlobalTypeCache();
								~GlobalTypeCache();

			status_t			Init();

	inline	bool				Lock();
	inline	void				Unlock();

			// cache must be locked
			Type*				GetType(const BString& name) const;
			status_t			AddType(const BString& name, Type* type);
			void				RemoveType(const BString& name);

			// cache locked by method
			void				RemoveTypes(image_id imageID);

private:
			struct TypeEntry;
			struct TypeEntryHashDefinition;

			typedef BOpenHashTable<TypeEntryHashDefinition> TypeTable;

private:
			BLocker				fLock;
			TypeTable*			fTypes;
};


class GlobalTypeLookup {
public:
	virtual						~GlobalTypeLookup();

	virtual	status_t			GetType(GlobalTypeCache* cache,
									const BString& name, Type*& _type) = 0;
									// returns a reference
};


bool
GlobalTypeCache::Lock()
{
	return fLock.Lock();
}


void
GlobalTypeCache::Unlock()
{
	fLock.Unlock();
}


#endif	// GLOBAL_TYPE_LOOKUP_H
