/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LINKS_LISTENER_H
#define PACKAGE_LINKS_LISTENER_H


#include <SupportDefs.h>


class Node;
class OldNodeAttributes;


class PackageLinksListener {
public:
	virtual						~PackageLinksListener();

	virtual	void				PackageLinkNodeAdded(Node* node) = 0;
	virtual	void				PackageLinkNodeRemoved(Node* node) = 0;
	virtual	void				PackageLinkNodeChanged(Node* node,
									uint32 statFields,
									const OldNodeAttributes& oldAttributes) = 0;
};


#endif	// PACKAGE_LINKS_LISTENER_H
