/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RESOLVABLE_FAMILY_H
#define RESOLVABLE_FAMILY_H


#include <util/khash.h>
#include <util/OpenHashTable.h>

#include "Resolvable.h"


class ResolvableFamily {
public:
			void				AddResolvable(Resolvable* resolvable,
									ResolvableDependencyList&
										dependenciesToUpdate);
			void				RemoveResolvable(Resolvable* resolvable,
									ResolvableDependencyList&
										dependenciesToUpdate);

			const char*			Name() const;

			bool				IsLastResolvable(Resolvable* resolvable) const;

			ResolvableFamily*&	HashLink()	{ return fHashLink; }

private:
			ResolvableFamily*	fHashLink;
			FamilyResolvableList fResolvables;
};


inline const char*
ResolvableFamily::Name() const
{
	Resolvable* head = fResolvables.Head();
	return head != NULL ? head->Name() : NULL;
}


inline bool
ResolvableFamily::IsLastResolvable(Resolvable* resolvable) const
{
	return fResolvables.Head() == resolvable
		&& fResolvables.Tail() == resolvable;
}


// #pragma mark - ResolvableFamilyHashDefinition


struct ResolvableFamilyHashDefinition {
	typedef const char*			KeyType;
	typedef	ResolvableFamily	ValueType;

	size_t HashKey(const char* key) const
	{
		return key != NULL ? hash_hash_string(key) : 0;
	}

	size_t Hash(const ResolvableFamily* value) const
	{
		return HashKey(value->Name());
	}

	bool Compare(const char* key, const ResolvableFamily* value) const
	{
		return strcmp(value->Name(), key) == 0;
	}

	ResolvableFamily*& GetLink(ResolvableFamily* value) const
	{
		return value->HashLink();
	}
};


typedef BOpenHashTable<ResolvableFamilyHashDefinition>
	ResolvableFamilyHashTable;


#endif	// RESOLVABLE_FAMILY_H
