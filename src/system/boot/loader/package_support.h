/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_SUPPORT_H
#define PACKAGE_SUPPORT_H


#include <dirent.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>


class Directory;


class PackageVolumeState : public DoublyLinkedListLinkImpl<PackageVolumeState> {
public:
								PackageVolumeState();
								~PackageVolumeState();

			status_t			SetTo(const char* stateName);
			void				Unset();

			const char*			Name() const
									{ return fName; }
			const char*			DisplayName() const;

			const char*			SystemPackage() const
									{ return fSystemPackage; }
			status_t			SetSystemPackage(const char* package);

			void				GetPackagePath(const char* name, char* path,
									size_t pathSize);

	static	bool				IsNewer(const PackageVolumeState* a,
									const PackageVolumeState* b);

private:
			char*				fName;
			char*				fDisplayName;
			char*				fSystemPackage;
};

typedef DoublyLinkedList<PackageVolumeState> PackageVolumeStateList;


class PackageVolumeInfo : public BReferenceable {
public:
			typedef PackageVolumeStateList StateList;

public:
								PackageVolumeInfo();
								~PackageVolumeInfo();

			status_t			SetTo(Directory* baseDirectory,
									const char* packagesPath);

			status_t			LoadOldStates();

			const StateList&	States() const
									{ return fStates; }

private:
			PackageVolumeState*	_AddState(const char* stateName);
			status_t			_InitState(Directory* packagesDirectory,
									DIR* dir, PackageVolumeState* state);
			status_t			_ParseActivatedPackagesFile(
									Directory* packagesDirectory,
									PackageVolumeState* state,
									char* packageName, size_t packageNameSize);

private:
			StateList			fStates;
			DIR*				fPackagesDir;
};


#endif	// PACKAGE_SUPPORT_H
