/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_H
#define PACKAGE_H


#include <package/hpkg/DataReader.h>
#include <package/PackageFlags.h>
#include <package/PackageArchitecture.h>

#include <WeakReferenceable.h>

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <util/StringHash.h>

#include <lock.h>

#include "Dependency.h"
#include "PackageNode.h"
#include "Resolvable.h"
#include "String.h"


using BPackageKit::BPackageArchitecture;
using BPackageKit::BHPKG::BAbstractBufferedDataReader;


class PackageLinkDirectory;
class PackagesDirectory;
class PackageSettings;
class Volume;
class Version;


class Package : public BWeakReferenceable,
	public DoublyLinkedListLinkImpl<Package> {
public:
								Package(::Volume* volume,
									PackagesDirectory* directory,
									dev_t deviceID, ino_t nodeID);
								~Package();

			status_t			Init(const char* fileName);
			status_t			Load(const PackageSettings& settings);

			::Volume*			Volume() const		{ return fVolume; }
			const String&		FileName() const	{ return fFileName; }

			void				SetName(const String& name);
			const String&		Name() const		{ return fName; }

			const String&		VersionedName() const
									{ return fVersionedName; }

			dev_t				DeviceID() const
									{ return fDeviceID; }
			ino_t				NodeID() const
									{ return fNodeID; }
			PackagesDirectory*	Directory() const
									{ return fPackagesDirectory; }

			void				SetInstallPath(const String& installPath);
			const String&		InstallPath() const	{ return fInstallPath; }

			void				SetVersion(::Version* version);
									// takes over object ownership
			::Version*			Version() const
									{ return fVersion; }

			void				SetFlags(uint32 flags)
									{ fFlags = flags; }
			uint32				Flags() const
									{ return fFlags; }

			void				SetArchitecture(
									BPackageArchitecture architecture)
									{ fArchitecture = architecture; }
			BPackageArchitecture Architecture() const
									{ return fArchitecture; }
			const char*			ArchitectureName() const;

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

			status_t			CreateDataReader(const PackageData& data,
									BAbstractBufferedDataReader*& _reader);

			const PackageNodeList& Nodes() const	{ return fNodes; }
			const ResolvableList& Resolvables() const
									{ return fResolvables; }
			const DependencyList& Dependencies() const
									{ return fDependencies; }

private:
			struct LoaderErrorOutput;
			struct LoaderContentHandler;
			struct LoaderContentHandlerV1;
			struct HeapReader;
			struct HeapReaderV1;
			struct HeapReaderV2;
			struct CachingPackageReader;

private:
			status_t			_Load(const PackageSettings& settings);
			bool				_InitVersionedName();

private:
			mutex				fLock;
			::Volume*			fVolume;
			PackagesDirectory*	fPackagesDirectory;
			String				fFileName;
			String				fName;
			String				fInstallPath;
			String				fVersionedName;
			::Version*			fVersion;
			uint32				fFlags;
			BPackageArchitecture fArchitecture;
			PackageLinkDirectory* fLinkDirectory;
			int					fFD;
			uint32				fOpenCount;
			HeapReader*			fHeapReader;
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
		return value->FileName().Hash();
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
