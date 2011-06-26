/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNPACKING_ATTRIBUTE_DIRECTORY_COOKIE_H
#define UNPACKING_ATTRIBUTE_DIRECTORY_COOKIE_H


#include "AttributeDirectoryCookie.h"
#include "AutoPackageAttributes.h"


struct dirent;

class PackageNode;
class PackageNodeAttribute;


class UnpackingAttributeDirectoryCookie : public AttributeDirectoryCookie {
public:
								UnpackingAttributeDirectoryCookie(
									PackageNode* packageNode);
	virtual						~UnpackingAttributeDirectoryCookie();

	static	status_t			Open(PackageNode* packageNode,
									AttributeDirectoryCookie*& _cookie);

	virtual	status_t			Read(dev_t volumeID, ino_t nodeID,
									struct dirent* buffer, size_t bufferSize,
									uint32* _count);
	virtual	status_t			Rewind();

private:
			PackageNode*		fPackageNode;
			PackageNodeAttribute* fAttribute;
			uint32				fState;
};


#endif	// UNPACKING_ATTRIBUTE_DIRECTORY_COOKIE_H
