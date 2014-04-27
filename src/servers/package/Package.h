/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef PACKAGE_H
#define PACKAGE_H


#include <set>

#include "PackageFile.h"


using namespace BPackageKit;


class Package {
public:
								Package(PackageFile* file);
								~Package();

			PackageFile*		File() const
									{ return fFile; }

			const node_ref&		NodeRef() const
									{ return fFile->NodeRef(); }
			const BString&		FileName() const
									{ return fFile->FileName(); }
			NotOwningEntryRef	EntryRef() const
									{ return fFile->EntryRef(); }

			const BPackageInfo & Info() const
									{ return fFile->Info(); }

			BString				RevisionedName() const
									{ return fFile->RevisionedName(); }
			BString				RevisionedNameThrows() const
									{ return fFile->RevisionedNameThrows(); }

			bool				IsActive() const
									{ return fActive; }
			void				SetActive(bool active)
									{ fActive = active; }

			Package*			Clone() const;

			Package*&			FileNameHashTableNext()
									{ return fFileNameHashTableNext; }
			Package*&			NodeRefHashTableNext()
									{ return fNodeRefHashTableNext; }

private:
			PackageFile*		fFile;
			bool				fActive;
			Package*			fFileNameHashTableNext;
			Package*			fNodeRefHashTableNext;
};


struct PackageFileNameHashDefinition {
	typedef const char*		KeyType;
	typedef	Package			ValueType;

	size_t HashKey(const char* key) const
	{
		return BString::HashValue(key);
	}

	size_t Hash(const Package* value) const
	{
		return HashKey(value->FileName());
	}

	bool Compare(const char* key, const Package* value) const
	{
		return value->FileName() == key;
	}

	Package*& GetLink(Package* value) const
	{
		return value->FileNameHashTableNext();
	}
};


struct PackageNodeRefHashDefinition {
	typedef node_ref		KeyType;
	typedef	Package			ValueType;

	size_t HashKey(const node_ref& key) const
	{
		return (size_t)key.device + 17 * (size_t)key.node;
	}

	size_t Hash(const Package* value) const
	{
		return HashKey(value->NodeRef());
	}

	bool Compare(const node_ref& key, const Package* value) const
	{
		return key == value->NodeRef();
	}

	Package*& GetLink(Package* value) const
	{
		return value->NodeRefHashTableNext();
	}
};


typedef BOpenHashTable<PackageFileNameHashDefinition> PackageFileNameHashTable;
typedef BOpenHashTable<PackageNodeRefHashDefinition> PackageNodeRefHashTable;
typedef std::set<Package*> PackageSet;


#endif	// PACKAGE_H
