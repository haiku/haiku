/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DOMAIN_H
#define PACKAGE_DOMAIN_H


#include <Referenceable.h>

#include <util/DoublyLinkedList.h>

#include "Package.h"


class NotificationListener;


class PackageDomain : public BReferenceable,
	public DoublyLinkedListLinkImpl<PackageDomain> {
public:
								PackageDomain();
								~PackageDomain();

			const char*			Path() const	{ return fPath; }
			int					DirectoryFD()	{ return fDirFD; }
			dev_t				DeviceID()		{ return fDeviceID; }
			ino_t				NodeID()		{ return fNodeID; }

			status_t			Init(const char* path);
			status_t			Prepare(NotificationListener* listener);
									// takes over ownership of the listener

			void				AddPackage(Package* package);
			void				RemovePackage(Package* package);

			Package*			FindPackage(const char* fileName) const;

			const PackageFileNameHashTable& Packages() const
									{ return fPackages; }

private:
			char*				fPath;
			int					fDirFD;
			dev_t				fDeviceID;
			ino_t				fNodeID;
			NotificationListener* fListener;
			PackageFileNameHashTable fPackages;
};


#endif	// PACKAGE_DOMAIN_H
