/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ENTRY_ATTRIBUTE_H
#define PACKAGE_ENTRY_ATTRIBUTE_H


#include "PackageData.h"


class PackageEntryAttribute {
public:
								PackageEntryAttribute(const char* name);

			const char*			Name() const			{ return fName; }
			uint32				Type() const			{ return fType; }

			PackageData&		Data()	{ return fData; }

			void				SetType(uint32 type)	{ fType = type; }

private:
			const char*			fName;
			uint32				fType;
			PackageData			fData;
};


#endif	// PACKAGE_ENTRY_ATTRIBUTE_H
