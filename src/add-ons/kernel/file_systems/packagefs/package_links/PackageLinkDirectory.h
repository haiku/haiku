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
	virtual	status_t			Init(Directory* parent, const String& name);

	virtual	timespec			ModifiedTime() const;

	virtual	status_t			OpenAttributeDirectory(
									AttributeDirectoryCookie*& _cookie);
	virtual	status_t			OpenAttribute(const StringKey& name,
									int openMode, AttributeCookie*& _cookie);

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
					PackageLinkSymlink(package, TYPE_INSTALLATION_LOCATION)
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

			status_t			_CreateOrUpdateLink(Link*& link,
									Package* package, Link::Type type,
									const String& name,
									PackageLinksListener* listener);
			void				_RemoveLink(Link* link,
									PackageLinksListener* listener);

private:
			timespec			fModifiedTime;
			PackageList			fPackages;
			Link*				fSelfLink;
			Link*				fSettingsLink;
			FamilyDependencyList fDependencyLinks;
};


#endif	// PACKAGE_LINK_DIRECTORY_H
