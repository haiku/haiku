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
class TypeLookupConstraints;


enum global_type_cache_scope {
	GLOBAL_TYPE_CACHE_SCOPE_GLOBAL,
	GLOBAL_TYPE_CACHE_SCOPE_COMPILATION_UNIT
};


class GlobalTypeCache : public BReferenceable {
public:
								GlobalTypeCache();
								~GlobalTypeCache();

			status_t			Init();

	inline	bool				Lock();
	inline	void				Unlock();

			// cache must be locked
			Type*				GetType(const BString& name) const;
			Type*				GetTypeByID(const BString& id) const;
			status_t			AddType(Type* type);
			void				RemoveType(Type* type);

			// cache locked by method
			void				RemoveTypes(image_id imageID);

private:
			struct TypeEntry;
			struct TypeEntryHashDefinitionByName;
			struct TypeEntryHashDefinitionByID;

			typedef BOpenHashTable<TypeEntryHashDefinitionByName> NameTable;
			typedef BOpenHashTable<TypeEntryHashDefinitionByID> IDTable;

private:
			BLocker				fLock;
			NameTable*			fTypesByName;
			IDTable*			fTypesByID;
};


class GlobalTypeLookup {
public:
	virtual						~GlobalTypeLookup();

	virtual	status_t			GetType(GlobalTypeCache* cache,
									const BString& name,
									const TypeLookupConstraints& constraints,
									Type*& _type) = 0;
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
