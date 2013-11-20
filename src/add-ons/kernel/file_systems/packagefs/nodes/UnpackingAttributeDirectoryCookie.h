/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNPACKING_ATTRIBUTE_DIRECTORY_COOKIE_H
#define UNPACKING_ATTRIBUTE_DIRECTORY_COOKIE_H


#include "AutoPackageAttributeDirectoryCookie.h"


class PackageNode;
class PackageNodeAttribute;


class UnpackingAttributeDirectoryCookie
	: public AutoPackageAttributeDirectoryCookie {
public:
								UnpackingAttributeDirectoryCookie(
									PackageNode* packageNode);
	virtual						~UnpackingAttributeDirectoryCookie();

	static	status_t			Open(PackageNode* packageNode,
									AttributeDirectoryCookie*& _cookie);

	virtual	status_t			Rewind();

protected:
	virtual	String				CurrentCustomAttributeName();
	virtual	String				NextCustomAttributeName();

private:
			PackageNode*		fPackageNode;
			PackageNodeAttribute* fAttribute;
};


#endif	// UNPACKING_ATTRIBUTE_DIRECTORY_COOKIE_H
