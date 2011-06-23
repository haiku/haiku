/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_FAMILY_H
#define PACKAGE_FAMILY_H


#include <Referenceable.h>

#include "Package.h"


/*!	A set of equally named and versioned packages.
	This should only happen when the same package is installed in multiple
	domains (e.g. common and home).
*/
class PackageFamily {
public:
								PackageFamily();
								~PackageFamily();

			status_t			Init(Package* package);

			const char*			Name() const	{ return fName; }

			void				AddPackage(Package* package);
			void				RemovePackage(Package* package);

			bool				IsEmpty() const
									{ return fPackages.IsEmpty(); }

			PackageFamily*&		HashNext()	{ return fHashNext; }

private:
			PackageFamily*		fHashNext;
			char*				fName;
			PackageList			fPackages;
};


struct PackageFamilyHashDefinition {
	typedef const char*		KeyType;
	typedef	PackageFamily	ValueType;

	size_t HashKey(const char* key) const
	{
		return hash_hash_string(key);
	}

	size_t Hash(const PackageFamily* value) const
	{
		return HashKey(value->Name());
	}

	bool Compare(const char* key, const PackageFamily* value) const
	{
		return strcmp(value->Name(), key) == 0;
	}

	PackageFamily*& GetLink(PackageFamily* value) const
	{
		return value->HashNext();
	}
};


typedef BOpenHashTable<PackageFamilyHashDefinition> PackageFamilyHashTable;


#endif	// PACKAGE_FAMILY_H
