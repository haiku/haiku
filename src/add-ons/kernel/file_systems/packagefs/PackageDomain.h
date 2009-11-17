/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DOMAIN_H
#define PACKAGE_DOMAIN_H


#include <util/DoublyLinkedList.h>

#include "Package.h"


class PackageDomain : public DoublyLinkedListLinkImpl<PackageDomain> {
public:
								PackageDomain();
								~PackageDomain();

			const char*			Path() const	{ return fPath; }
			int					DirectoryFD()	{ return fDirFD; }

			status_t			Init(const char* path);
			status_t			Prepare();

			void				AddPackage(Package* package);
			void				RemovePackage(Package* package);

			const PackageHashTable& Packages() const
									{ return fPackages; }

private:
			char*				fPath;
			int					fDirFD;
			PackageHashTable	fPackages;
};


#endif	// PACKAGE_DOMAIN_H
