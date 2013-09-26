/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef PACKAGE_H
#define PACKAGE_H


#include <set>

#include <Entry.h>
#include <Node.h>
#include <package/PackageInfo.h>

#include <util/OpenHashTable.h>


using namespace BPackageKit;


class Package {
public:
								Package();
								~Package();

			status_t			Init(const entry_ref& entryRef);

			const node_ref&		NodeRef() const
									{ return fNodeRef; }
			const BString&		FileName() const
									{ return fFileName; }

			const BPackageInfo & Info() const
									{ return fInfo; }

			BString				RevisionedName() const;
			BString				RevisionedNameThrows() const;

			bool				IsActive() const
									{ return fActive; }
			void				SetActive(bool active)
									{ fActive = active; }

			int32				EntryCreatedIgnoreLevel() const
									{ return fIgnoreEntryCreated; }
			void				IncrementEntryCreatedIgnoreLevel()
									{ fIgnoreEntryCreated++; }
			void				DecrementEntryCreatedIgnoreLevel()
									{ fIgnoreEntryCreated--; }

			int32				EntryRemovedIgnoreLevel() const
									{ return fIgnoreEntryRemoved; }
			void				IncrementEntryRemovedIgnoreLevel()
									{ fIgnoreEntryRemoved++; }
			void				DecrementEntryRemovedIgnoreLevel()
									{ fIgnoreEntryRemoved--; }

			Package*&			FileNameHashTableNext()
									{ return fFileNameHashTableNext; }
			Package*&			NodeRefHashTableNext()
									{ return fNodeRefHashTableNext; }

private:
			node_ref			fNodeRef;
			BString				fFileName;
			BPackageInfo		fInfo;
			bool				fActive;
			Package*			fFileNameHashTableNext;
			Package*			fNodeRefHashTableNext;
			int32				fIgnoreEntryCreated;
			int32				fIgnoreEntryRemoved;
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
