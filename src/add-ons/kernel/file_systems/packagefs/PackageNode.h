/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_NODE_H
#define PACKAGE_NODE_H


#include <SupportDefs.h>

#include <util/SinglyLinkedList.h>


class PackageDirectory;


class PackageNode : public SinglyLinkedListLinkImpl<PackageNode> {
public:
								PackageNode();
	virtual						~PackageNode();

			PackageDirectory*	Parent() const	{ return fParent; }
			const char*			Name() const	{ return fName; }

	virtual	status_t			Init(PackageDirectory* parent,
									const char* name);

			void				SetMode(mode_t mode)	{ fMode = mode; }
			mode_t				Mode() const			{ return fMode; }

private:
			PackageDirectory*	fParent;
			char*				fName;
			mode_t				fMode;
};


typedef SinglyLinkedList<PackageNode> PackageNodeList;


#endif	// PACKAGE_NODE_H
