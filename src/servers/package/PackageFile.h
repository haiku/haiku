/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef PACKAGE_FILE_H
#define PACKAGE_FILE_H


#include <Node.h>
#include <package/PackageInfo.h>

#include <NotOwningEntryRef.h>
#include <Referenceable.h>
#include <util/OpenHashTable.h>


using namespace BPackageKit;


class PackageFileManager;


class PackageFile : public BReferenceable {
public:
								PackageFile();
								~PackageFile();

			status_t			Init(const entry_ref& entryRef,
									PackageFileManager* owner);

			const node_ref&		NodeRef() const
									{ return fNodeRef; }
			const BString&		FileName() const
									{ return fFileName; }

			const node_ref&		DirectoryRef() const
									{ return fDirectoryRef; }
			void				SetDirectoryRef(const node_ref& directoryRef)
									{ fDirectoryRef = directoryRef; }

			NotOwningEntryRef	EntryRef() const;

			const BPackageInfo & Info() const
									{ return fInfo; }

			BString				RevisionedName() const;
			BString				RevisionedNameThrows() const;

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

			PackageFile*&		EntryRefHashTableNext()
									{ return fEntryRefHashTableNext; }

protected:
	virtual	void				LastReferenceReleased();

private:
			node_ref			fNodeRef;
			node_ref			fDirectoryRef;
			BString				fFileName;
			BPackageInfo		fInfo;
			PackageFile*		fEntryRefHashTableNext;
			PackageFileManager*	fOwner;
			int32				fIgnoreEntryCreated;
			int32				fIgnoreEntryRemoved;
};


inline NotOwningEntryRef
PackageFile::EntryRef() const
{
	return NotOwningEntryRef(fDirectoryRef, fFileName);
}


struct PackageFileEntryRefHashDefinition {
	typedef entry_ref		KeyType;
	typedef	PackageFile		ValueType;

	size_t HashKey(const entry_ref& key) const
	{
		size_t hash = BString::HashValue(key.name);
		hash ^= (size_t)key.device;
		hash ^= (size_t)key.directory;
		return hash;
	}

	size_t Hash(const PackageFile* value) const
	{
		return HashKey(value->EntryRef());
	}

	bool Compare(const entry_ref& key, const PackageFile* value) const
	{
		return value->EntryRef() == key;
	}

	PackageFile*& GetLink(PackageFile* value) const
	{
		return value->EntryRefHashTableNext();
	}
};


typedef BOpenHashTable<PackageFileEntryRefHashDefinition>
	PackageFileEntryRefHashTable;


#endif	// PACKAGE_FILE_H
