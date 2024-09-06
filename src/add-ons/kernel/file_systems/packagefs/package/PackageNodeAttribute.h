/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_NODE_ATTRIBUTE_H
#define PACKAGE_NODE_ATTRIBUTE_H


#include <util/SinglyLinkedList.h>

#include "PackageData.h"

#include "String.h"


class PackageNode;


class PackageNodeAttribute final
	: public SinglyLinkedListLinkImpl<PackageNodeAttribute> {
public:
	static	void*				operator new(size_t size);
	static	void				operator delete(void* block);

								PackageNodeAttribute(uint32 type,
									const PackageData& data);
								~PackageNodeAttribute();

			const String&		Name() const	{ return fName; }
			uint32				Type() const	{ return fType; }
			const PackageData&	Data() const	{ return fData; }

			void				Init(const String& name);

			void				SetIndexCookie(void* cookie)
									{ fIndexCookie = cookie; }
			void*				IndexCookie() const
									{ return fIndexCookie; }

protected:
			String				fName;
			void*				fIndexCookie;
			PackageData			fData;
			uint32				fType;
};


typedef SinglyLinkedList<PackageNodeAttribute> PackageNodeAttributeList;


#endif	// PACKAGE_NODE_ATTRIBUTE_H
