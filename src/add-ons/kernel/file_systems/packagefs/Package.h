/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_H
#define PACKAGE_H


#include <Referenceable.h>

#include <util/khash.h>
#include <util/OpenHashTable.h>

#include <lock.h>

#include "PackageNode.h"


class PackageDomain;


class Package : public BReferenceable {
public:
								Package(PackageDomain* domain, dev_t deviceID,
									ino_t nodeID);
								~Package();

			status_t			Init(const char* name);

			PackageDomain*		Domain() const	{ return fDomain; }
			const char*			Name() const	{ return fName; }

			Package*&			HashTableNext() { return fHashTableNext; }

			void				AddNode(PackageNode* node);

			int					Open();
			void				Close();

			const PackageNodeList&	Nodes() const
									{ return fNodes; }

private:
			mutex				fLock;
			PackageDomain*		fDomain;
			char*				fName;
			int					fFD;
			uint32				fOpenCount;
			Package*			fHashTableNext;
			ino_t				fNodeID;
			dev_t				fDeviceID;
			PackageNodeList		fNodes;
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


struct PackageHashDefinition {
	typedef const char*		KeyType;
	typedef	Package			ValueType;

	size_t HashKey(const char* key) const
	{
		return hash_hash_string(key);
	}

	size_t Hash(const Package* value) const
	{
		return HashKey(value->Name());
	}

	bool Compare(const char* key, const Package* value) const
	{
		return strcmp(value->Name(), key) == 0;
	}

	Package*& GetLink(Package* value) const
	{
		return value->HashTableNext();
	}
};


typedef BOpenHashTable<PackageHashDefinition> PackageHashTable;


#endif	// PACKAGE_H
