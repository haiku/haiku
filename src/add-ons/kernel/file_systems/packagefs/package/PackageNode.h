/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_NODE_H
#define PACKAGE_NODE_H


#include <sys/stat.h>

#include <WeakReferenceable.h>

#include <util/SinglyLinkedList.h>

#include "IndexedAttributeOwner.h"
#include "PackageNodeAttribute.h"
#include "StringKey.h"


class AttributeIndexer;
class Package;
class PackageDirectory;


class PackageNode : public BReferenceable, public IndexedAttributeOwner,
	public SinglyLinkedListLinkImpl<PackageNode> {
public:
								PackageNode(Package* package, mode_t mode);
	virtual						~PackageNode();

			BReference<Package>		GetPackage() const;
									// Since PackageNode does only hold a
									// reference to the package between
									// VFSInit() and VFSUninit(), the caller
									// must otherwise make sure the package
									// still exists.
			PackageDirectory*	Parent() const		{ return fParent; }
			const String&		Name() const		{ return fName; }

	virtual	status_t			Init(PackageDirectory* parent,
									const String& name);

	virtual	status_t			VFSInit(dev_t deviceID, ino_t nodeID);
	virtual	void				VFSUninit();
									// base class versions must be called

			mode_t				Mode() const			{ return fMode; }

			uid_t				UserID() const			{ return fUserID; }
			void				SetUserID(uid_t id)		{ fUserID = id; }

			gid_t				GroupID() const			{ return fGroupID; }
			void				SetGroupID(gid_t id)	{ fGroupID = id; }

			void				SetModifiedTime(const timespec& time)
									{ fModifiedTime = time; }
			const timespec&		ModifiedTime() const
									{ return fModifiedTime; }

	virtual	off_t				FileSize() const;

			void				AddAttribute(PackageNodeAttribute* attribute);
			void				RemoveAttribute(
									PackageNodeAttribute* attribute);

			const PackageNodeAttributeList& Attributes() const
									{ return fAttributes; }

			PackageNodeAttribute* FindAttribute(const StringKey& name) const;

	virtual	void				UnsetIndexCookie(void* attributeCookie);

	inline	void*				IndexCookieForAttribute(const StringKey& name)
									const;

			bool				HasPrecedenceOver(const PackageNode* other) const;

			// conceptually protected, but actually declaring it so causes
			// compilation issues when used with MethodDeleter in subclasses
			void				NonVirtualVFSUninit()
									{ PackageNode::VFSUninit(); }
									// service for derived classes, e.g. for use
									// with MethodDeleter

protected:
	mutable BWeakReference<Package> fPackage;
			PackageDirectory*	fParent;
			String				fName;
			mode_t				fMode;
			uid_t				fUserID;
			gid_t				fGroupID;
			timespec			fModifiedTime;
			PackageNodeAttributeList fAttributes;
};


void*
PackageNode::IndexCookieForAttribute(const StringKey& name) const
{
	PackageNodeAttribute* attribute = FindAttribute(name);
	return attribute != NULL ? attribute->IndexCookie() : NULL;
}


typedef SinglyLinkedList<PackageNode> PackageNodeList;


#endif	// PACKAGE_NODE_H
