/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_NODE_ATTRIBUTE_H
#define PACKAGE_NODE_ATTRIBUTE_H


#include <util/DoublyLinkedList.h>

#include <package/hpkg/PackageData.h>

#include "String.h"


using BPackageKit::BHPKG::BPackageData;

class PackageNode;


class PackageNodeAttribute
	: public DoublyLinkedListLinkImpl<PackageNodeAttribute> {
public:
								PackageNodeAttribute(uint32 type,
									const BPackageData& data);
								~PackageNodeAttribute();

			const String&		Name() const	{ return fName; }
			uint32				Type() const	{ return fType; }
			const BPackageData&	Data() const	{ return fData; }

			void				Init(const String& name);

			void				SetIndexCookie(void* cookie)
									{ fIndexCookie = cookie; }
			void*				IndexCookie() const
									{ return fIndexCookie; }

protected:
			BPackageData		fData;
			String				fName;
			void*				fIndexCookie;
			uint32				fType;
};


typedef DoublyLinkedList<PackageNodeAttribute> PackageNodeAttributeList;


#endif	// PACKAGE_NODE_ATTRIBUTE_H
