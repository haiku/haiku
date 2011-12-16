/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef AUTO_PACKAGE_ATTRIBUTES_H
#define AUTO_PACKAGE_ATTRIBUTES_H


#include <SupportDefs.h>


class AttributeCookie;
class Package;


enum AutoPackageAttribute {
	AUTO_PACKAGE_ATTRIBUTE_ENUM_FIRST,
	AUTO_PACKAGE_ATTRIBUTE_PACKAGE = AUTO_PACKAGE_ATTRIBUTE_ENUM_FIRST,

	AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT
};


struct AutoPackageAttributes {
	static	bool				AttributeForName(const char* name,
									AutoPackageAttribute& _attribute);
	static	const char*			NameForAttribute(
									AutoPackageAttribute attribute);
	static	const void*			GetAttributeValue(const Package* package,
									AutoPackageAttribute attribute,
									off_t& _size, uint32& _type);

	static	status_t			OpenCookie(Package* package, const char* name,
									int openMode, AttributeCookie*& _cookie);
};


#endif	// AUTO_PACKAGE_ATTRIBUTES_H
