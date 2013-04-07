/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DOMAIN_H
#define PACKAGE_DOMAIN_H


#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "Package.h"


struct stat;

class Volume;


class PackageDomain : public BReferenceable,
	public DoublyLinkedListLinkImpl<PackageDomain> {
public:
								PackageDomain(::Volume* volume);
								~PackageDomain();

			::Volume*			Volume() const	{ return fVolume; }
			const char*			Path() const	{ return fPath; }
			int					DirectoryFD()	{ return fDirFD; }
			dev_t				DeviceID()		{ return fDeviceID; }
			ino_t				NodeID()		{ return fNodeID; }

			status_t			Init(const char* path, struct stat* _st);

			void				AddPackage(Package* package);
			void				RemovePackage(Package* package);

			Package*			FindPackage(const char* fileName) const;

			const PackageFileNameHashTable& Packages() const
									{ return fPackages; }

private:
			::Volume*			fVolume;
			char*				fPath;
			int					fDirFD;
			dev_t				fDeviceID;
			ino_t				fNodeID;
			PackageFileNameHashTable fPackages;
};


#endif	// PACKAGE_DOMAIN_H
