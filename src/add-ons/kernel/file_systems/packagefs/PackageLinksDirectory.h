/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LINKS_DIRECTORY_H
#define PACKAGE_LINKS_DIRECTORY_H


#include "Directory.h"
#include "PackageFamily.h"


class PackageLinksDirectory : public Directory {
public:
								PackageLinksDirectory();
	virtual						~PackageLinksDirectory();

	virtual	status_t			Init(Directory* parent, const char* name);

	virtual	mode_t				Mode() const;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;
	virtual	timespec			ModifiedTime() const;
	virtual	off_t				FileSize() const;

	virtual	status_t			OpenAttributeDirectory(
									AttributeDirectoryCookie*& _cookie);
	virtual	status_t			OpenAttribute(const char* name, int openMode,
									AttributeCookie*& _cookie);

			status_t			AddPackage(Package* package);
			void				RemovePackage(Package* package);

private:
			timespec			fModifiedTime;
			PackageFamilyHashTable fPackageFamilies;
};


#endif	// PACKAGE_LINKS_DIRECTORY_H
