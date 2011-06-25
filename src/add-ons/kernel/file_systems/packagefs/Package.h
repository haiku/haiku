/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_H
#define PACKAGE_H


#include <Referenceable.h>

#include <util/DoublyLinkedList.h>
#include <util/khash.h>
#include <util/OpenHashTable.h>

#include <lock.h>

#include "Dependency.h"
#include "PackageNode.h"
#include "Resolvable.h"


class PackageDomain;
class PackageLinkDirectory;
class Version;


class Package : public BReferenceable,
	public DoublyLinkedListLinkImpl<Package> {
public:
								Package(PackageDomain* domain, dev_t deviceID,
									ino_t nodeID);
								~Package();

			status_t			Init(const char* fileName);

			PackageDomain*		Domain() const		{ return fDomain; }
			const char*			FileName() const	{ return fFileName; }

			status_t			SetName(const char* name);
			const char*			Name() const		{ return fName; }

			void				SetVersion(::Version* version);
									// takes over object ownership
			::Version*			Version() const
									{ return fVersion; }

			void				SetLinkDirectory(
									PackageLinkDirectory* linkDirectory)
									{ fLinkDirectory = linkDirectory; }
			PackageLinkDirectory* LinkDirectory() const
									{ return fLinkDirectory; }

			Package*&			FileNameHashTableNext()
									{ return fFileNameHashTableNext; }

			void				AddNode(PackageNode* node);
			void				AddResolvable(Resolvable* resolvable);
			void				AddDependency(Dependency* dependency);

			int					Open();
			void				Close();

			const PackageNodeList& Nodes() const	{ return fNodes; }
			const ResolvableList& Resolvables() const
									{ return fResolvables; }
			const DependencyList& Dependencies() const
									{ return fDependencies; }

private:
			mutex				fLock;
			PackageDomain*		fDomain;
			char*				fFileName;
			char*				fName;
			::Version*			fVersion;
			PackageLinkDirectory* fLinkDirectory;
			int					fFD;
			uint32				fOpenCount;
			Package*			fFileNameHashTableNext;
			ino_t				fNodeID;
			dev_t				fDeviceID;
			PackageNodeList		fNodes;
			ResolvableList		fResolvables;
			DependencyList		fDependencies;
};


struct PackageCloser {
	PackageCloser(Package* package)
		:
		fPackage(package)
	{
	}

	~PackageCloser()
	{
		if (fPackage != NULL)
			fPackage->Close();
	}

	void Detach()
	{
		fPackage = NULL;
	}

private:
	Package*	fPackage;
};


struct PackageFileNameHashDefinition {
	typedef const char*		KeyType;
	typedef	Package			ValueType;

	size_t HashKey(const char* key) const
	{
		return hash_hash_string(key);
	}

	size_t Hash(const Package* value) const
	{
		return HashKey(value->FileName());
	}

	bool Compare(const char* key, const Package* value) const
	{
		return strcmp(value->FileName(), key) == 0;
	}

	Package*& GetLink(Package* value) const
	{
		return value->FileNameHashTableNext();
	}
};


typedef BOpenHashTable<PackageFileNameHashDefinition> PackageFileNameHashTable;
typedef DoublyLinkedList<Package> PackageList;


#endif	// PACKAGE_H
