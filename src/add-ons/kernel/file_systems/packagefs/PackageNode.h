/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_NODE_H
#define PACKAGE_NODE_H


#include <sys/stat.h>

#include <SupportDefs.h>

#include <util/SinglyLinkedList.h>


class PackageDirectory;


class PackageNode : public SinglyLinkedListLinkImpl<PackageNode> {
public:
								PackageNode(mode_t mode);
	virtual						~PackageNode();

			PackageDirectory*	Parent() const	{ return fParent; }
			const char*			Name() const	{ return fName; }

	virtual	status_t			Init(PackageDirectory* parent,
									const char* name);

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

protected:
			PackageDirectory*	fParent;
			char*				fName;
			mode_t				fMode;
			uid_t				fUserID;
			gid_t				fGroupID;
			timespec			fModifiedTime;
};


typedef SinglyLinkedList<PackageNode> PackageNodeList;


#endif	// PACKAGE_NODE_H
