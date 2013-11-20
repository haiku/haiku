/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingAttributeDirectoryCookie.h"

#include "DebugSupport.h"
#include "PackageNode.h"


UnpackingAttributeDirectoryCookie::UnpackingAttributeDirectoryCookie(
	PackageNode* packageNode)
	:
	AutoPackageAttributeDirectoryCookie(),
	fPackageNode(packageNode),
	fAttribute(NULL)
{
	if (fPackageNode != NULL) {
		fPackageNode->AcquireReference();
		fAttribute = fPackageNode->Attributes().Head();
	}
}


UnpackingAttributeDirectoryCookie::~UnpackingAttributeDirectoryCookie()
{
	if (fPackageNode != NULL)
		fPackageNode->ReleaseReference();
}


/*static*/ status_t
UnpackingAttributeDirectoryCookie::Open(PackageNode* packageNode,
	AttributeDirectoryCookie*& _cookie)
{
	UnpackingAttributeDirectoryCookie* cookie = new(std::nothrow)
		UnpackingAttributeDirectoryCookie(packageNode);
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


status_t
UnpackingAttributeDirectoryCookie::Rewind()
{
	if (fPackageNode != NULL)
		fAttribute = fPackageNode->Attributes().Head();

	return AutoPackageAttributeDirectoryCookie::Rewind();
}


String
UnpackingAttributeDirectoryCookie::CurrentCustomAttributeName()
{
	return fAttribute != NULL ? fAttribute->Name() : String();
}


String
UnpackingAttributeDirectoryCookie::NextCustomAttributeName()
{
	if (fAttribute != NULL)
		fAttribute = fPackageNode->Attributes().GetNext(fAttribute);
	return fAttribute != NULL ? fAttribute->Name() : String();
}
