/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LINK_DIRECTORY_H
#define PACKAGE_LINK_DIRECTORY_H


#include "Directory.h"
#include "Package.h"
#include "PackageLinkSymlink.h"


class PackageLinksListener;


class PackageLinkDirectory : public Directory {
public:
								PackageLinkDirectory();
	virtual						~PackageLinkDirectory();

			status_t			Init(Directory* parent, Package* package);
	virtual	status_t			Init(Directory* parent, const char* name,
									uint32 flags);

	virtual	mode_t				Mode() const;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;
	virtual	timespec			ModifiedTime() const;
	virtual	off_t				FileSize() const;

	virtual	status_t			OpenAttributeDirectory(
									AttributeDirectoryCookie*& _cookie);
	virtual	status_t			OpenAttribute(const char* name, int openMode,
									AttributeCookie*& _cookie);

			void				AddPackage(Package* package,
									PackageLinksListener* listener);
			void				RemovePackage(Package* package,
									PackageLinksListener* listener);
			void				UpdatePackageDependencies(Package* package,
									PackageLinksListener* listener);

			bool				IsEmpty() const
									{ return fPackages.IsEmpty(); }

private:
			typedef PackageLinkSymlink Link;

			struct DependencyLink : public PackageLinkSymlink {
				DependencyLink(Package* package)
					:
					PackageLinkSymlink(package)
				{
				}

				DoublyLinkedListLink<DependencyLink> fPackageLinkDirectoryLink;
			};

			typedef DoublyLinkedList<DependencyLink,
				DoublyLinkedListMemberGetLink<DependencyLink,
					&DependencyLink::fPackageLinkDirectoryLink> >
				FamilyDependencyList;

private:
			status_t			_Update(PackageLinksListener* listener);
			status_t			_UpdateDependencies(
									PackageLinksListener* listener);

private:
			timespec			fModifiedTime;
			PackageList			fPackages;
			Link*				fSelfLink;
			FamilyDependencyList fDependencyLinks;
};


#endif	// PACKAGE_LINK_DIRECTORY_H
