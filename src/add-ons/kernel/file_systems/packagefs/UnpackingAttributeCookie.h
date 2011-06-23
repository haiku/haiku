/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNPACKING_ATTRIBUTE_COOKIE_H
#define UNPACKING_ATTRIBUTE_COOKIE_H


#include "AttributeCookie.h"


class Package;
class PackageNode;
class PackageNodeAttribute;


class UnpackingAttributeCookie : public AttributeCookie {
public:
								UnpackingAttributeCookie(
									PackageNode* packageNode,
									PackageNodeAttribute* attribute,
									int openMode);
	virtual						~UnpackingAttributeCookie();

	static	status_t			Open(PackageNode* packageNode, const char* name,
									int openMode, AttributeCookie*& _cookie);

	virtual	status_t			Close();
	virtual	status_t			ReadAttribute(off_t offset, void* buffer,
									size_t* bufferSize);
	virtual	status_t			ReadAttributeStat(struct stat* st);

private:
			PackageNode*		fPackageNode;
			Package*			fPackage;
			PackageNodeAttribute* fAttribute;
			int					fOpenMode;
};


#endif	// UNPACKING_ATTRIBUTE_COOKIE_H
