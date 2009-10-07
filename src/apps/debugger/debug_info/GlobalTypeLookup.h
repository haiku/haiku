/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GLOBAL_TYPE_LOOKUP_H
#define GLOBAL_TYPE_LOOKUP_H


#include <Locker.h>

#include <Referenceable.h>
#include <util/OpenHashTable.h>


class BString;
class Type;


class GlobalTypeLookupContext : public Referenceable {
public:
								GlobalTypeLookupContext();
								~GlobalTypeLookupContext();

			status_t			Init();

	inline	bool				Lock();
	inline	void				Unlock();

			// context must be locked
			Type*				CachedType(const BString& name) const;
			status_t			AddCachedType(const BString& name, Type* type);
			void				RemoveCachedType(const BString& name);

private:
			struct TypeEntry;
			struct TypeEntryHashDefinition;

			typedef BOpenHashTable<TypeEntryHashDefinition> TypeTable;

private:
			BLocker				fLock;
			TypeTable*			fCachedTypes;
};


class GlobalTypeLookup {
public:
								~GlobalTypeLookup();

	virtual	status_t			GetType(GlobalTypeLookupContext* context,
									const BString& name, Type*& _type) = 0;
									// returns a reference
};


bool
GlobalTypeLookupContext::Lock()
{
	return fLock.Lock();
}


void
GlobalTypeLookupContext::Unlock()
{
	fLock.Unlock();
}


#endif	// GLOBAL_TYPE_LOOKUP_H
