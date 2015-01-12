/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RESOLVABLE_FAMILY_H
#define RESOLVABLE_FAMILY_H


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

			bool				ResolveDependency(Dependency* dependency);

			String				Name() const;

			bool				IsLastResolvable(Resolvable* resolvable) const;

			ResolvableFamily*&	HashLink()	{ return fHashLink; }

private:
			ResolvableFamily*	fHashLink;
			FamilyResolvableList fResolvables;
};


inline String
ResolvableFamily::Name() const
{
	Resolvable* head = fResolvables.Head();
	return head != NULL ? head->Name() : String();
}


inline bool
ResolvableFamily::IsLastResolvable(Resolvable* resolvable) const
{
	return fResolvables.Head() == resolvable
		&& fResolvables.Tail() == resolvable;
}


// #pragma mark - ResolvableFamilyHashDefinition


struct ResolvableFamilyHashDefinition {
	typedef String				KeyType;
	typedef	ResolvableFamily	ValueType;

	size_t HashKey(const String& key) const
	{
		return key.Hash();
	}

	size_t Hash(const ResolvableFamily* value) const
	{
		return value->Name().Hash();
	}

	bool Compare(const String& key, const ResolvableFamily* value) const
	{
		return key == value->Name();
	}

	ResolvableFamily*& GetLink(ResolvableFamily* value) const
	{
		return value->HashLink();
	}
};


typedef BOpenHashTable<ResolvableFamilyHashDefinition>
	ResolvableFamilyHashTable;


#endif	// RESOLVABLE_FAMILY_H
