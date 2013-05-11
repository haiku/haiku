/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LINKS_DIRECTORY_H
#define PACKAGE_LINKS_DIRECTORY_H


#include "Directory.h"


class Package;
class PackageLinksListener;


class PackageLinksDirectory : public Directory {
public:
								PackageLinksDirectory();
	virtual						~PackageLinksDirectory();

	virtual	timespec			ModifiedTime() const;

			void				SetListener(PackageLinksListener* listener)
									{ fListener = listener; }

			status_t			AddPackage(Package* package);
			void				RemovePackage(Package* package);
			void				UpdatePackageDependencies(Package* package);

private:
			timespec			fModifiedTime;
			PackageLinksListener* fListener;
};


#endif	// PACKAGE_LINKS_DIRECTORY_H
