/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEPENDENCY_FAMILY_H
#define DEPENDENCY_FAMILY_H


#include <util/OpenHashTable.h>

#include "Dependency.h"


class DependencyFamily {
public:
			void				AddDependency(Dependency* dependency);
			void				RemoveDependency(Dependency* dependency);

			void				AddDependenciesToList(
									ResolvableDependencyList& list) const;

			String				Name() const;

			bool				IsLastDependency(Dependency* dependency) const;

			DependencyFamily*&	HashLink()	{ return fHashLink; }

private:
			DependencyFamily*	fHashLink;
			FamilyDependencyList fDependencies;
};


inline void
DependencyFamily::AddDependency(Dependency* dependency)
{
	fDependencies.Add(dependency);
	dependency->SetFamily(this);
}


inline void
DependencyFamily::RemoveDependency(Dependency* dependency)
{
	dependency->SetFamily(NULL);
	fDependencies.Remove(dependency);
}


inline void
DependencyFamily::AddDependenciesToList(ResolvableDependencyList& list) const
{
	for (FamilyDependencyList::ConstIterator it = fDependencies.GetIterator();
			Dependency* dependency = it.Next();) {
		list.Add(dependency);
	}
}


inline String
DependencyFamily::Name() const
{
	Dependency* head = fDependencies.Head();
	return head != NULL ? head->Name() : String();
}


inline bool
DependencyFamily::IsLastDependency(Dependency* dependency) const
{
	return fDependencies.Head() == dependency
		&& fDependencies.Tail() == dependency;
}


// #pragma mark - DependencyFamilyHashDefinition


struct DependencyFamilyHashDefinition {
	typedef String				KeyType;
	typedef	DependencyFamily	ValueType;

	size_t HashKey(const String& key) const
	{
		return key.Hash();
	}

	size_t Hash(const DependencyFamily* value) const
	{
		return value->Name().Hash();
	}

	bool Compare(const String& key, const DependencyFamily* value) const
	{
		return key == value->Name();
	}

	DependencyFamily*& GetLink(DependencyFamily* value) const
	{
		return value->HashLink();
	}
};


typedef BOpenHashTable<DependencyFamilyHashDefinition>
	DependencyFamilyHashTable;


#endif	// DEPENDENCY_FAMILY_H
