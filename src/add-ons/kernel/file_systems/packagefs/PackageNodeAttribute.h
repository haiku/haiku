/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_NODE_ATTRIBUTE_H
#define PACKAGE_NODE_ATTRIBUTE_H


#include <util/DoublyLinkedList.h>

#include <package/hpkg/PackageData.h>


using BPackageKit::BHaikuPackage::BPrivate::PackageData;

class PackageNode;


class PackageNodeAttribute
	: public DoublyLinkedListLinkImpl<PackageNodeAttribute> {
public:
								PackageNodeAttribute(PackageNode* parent,
									uint32 type, const PackageData& data);
								~PackageNodeAttribute();

			PackageNode*		Parent() const	{ return fParent; }
			const char*			Name() const	{ return fName; }
			uint32				Type() const	{ return fType; }
			const PackageData&	Data() const	{ return fData; }

			status_t			Init(const char* name);


protected:
			PackageData			fData;
			PackageNode*		fParent;
			char*				fName;
			uint32				fType;
};


typedef DoublyLinkedList<PackageNodeAttribute> PackageNodeAttributeList;


#endif	// PACKAGE_NODE_ATTRIBUTE_H
