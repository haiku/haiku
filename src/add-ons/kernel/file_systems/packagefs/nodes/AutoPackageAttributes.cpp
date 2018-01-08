/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "AutoPackageAttributes.h"

#include <algorithm>
#include <new>

#include <TypeConstants.h>

#include <AutoDeleter.h>

#include "AttributeCookie.h"
#include "DebugSupport.h"
#include "Package.h"
#include "StringConstants.h"


class AutoPackageAttributeCookie : public AttributeCookie {
public:
	AutoPackageAttributeCookie(Package* package, AutoPackageAttribute attribute)
		:
		fPackage(package),
		fAttribute(attribute)
	{
		fPackage->AcquireReference();
	}

	virtual ~AutoPackageAttributeCookie()
	{
		fPackage->ReleaseReference();
	}

	virtual status_t ReadAttribute(off_t offset, void* buffer,
		size_t* bufferSize)
	{
		// get the attribute
		off_t size;
		uint32 type;
		const void* value = AutoPackageAttributes::GetAttributeValue(fPackage,
			fAttribute, size, type);
		if (value == NULL)
			return B_BAD_VALUE;

		// check and clamp offset and size
		if (offset < 0 || offset > size)
			return B_BAD_VALUE;

		size_t toCopy = *bufferSize;
		if (offset + (off_t)toCopy > size)
			toCopy = size - offset;

		if (toCopy > 0 && user_memcpy(buffer, (const uint8*)value + offset,
			toCopy) < B_OK) {
			return B_BAD_ADDRESS;
		}

		*bufferSize = toCopy;
		return B_OK;
	}

	virtual status_t ReadAttributeStat(struct stat* st)
	{
		if (AutoPackageAttributes::GetAttributeValue(fPackage, fAttribute,
			st->st_size, st->st_type) == NULL) {
			return B_BAD_VALUE;
		}

		return B_OK;
	}

private:
	Package*				fPackage;
	AutoPackageAttribute	fAttribute;
};


/*static*/ bool
AutoPackageAttributes::AttributeForName(const StringKey& name,
	AutoPackageAttribute& _attribute)
{
	for (int i = 0; i < AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT; i++) {
		if (name == StringConstants::Get().kAutoPackageAttributeNames[i]) {
			_attribute = (AutoPackageAttribute)i;
			return true;
		}
	}

	return false;
}


/*static*/ const String&
AutoPackageAttributes::NameForAttribute(AutoPackageAttribute attribute)
{
	ASSERT(attribute >= 0 && attribute < AUTO_PACKAGE_ATTRIBUTE_ENUM_COUNT);
	return StringConstants::Get().kAutoPackageAttributeNames[attribute];
}


/*static*/ const void*
AutoPackageAttributes::GetAttributeValue(const Package* package,
	AutoPackageAttribute attribute, off_t& _size, uint32& _type)
{
	switch (attribute) {
		case AUTO_PACKAGE_ATTRIBUTE_PACKAGE:
		{
			const char* value = package->VersionedName();
			_size = strlen(value) + 1;
			_type = B_STRING_TYPE;
			return value;
		}

		case AUTO_PACKAGE_ATTRIBUTE_PACKAGE_FILE:
		{
			const char* value = package->FileName();
			_size = strlen(value) + 1;
			_type = B_STRING_TYPE;
			return value;
		}

		default:
			return NULL;
	}
}


/*static*/ status_t
AutoPackageAttributes::OpenCookie(Package* package, const StringKey& name,
	int openMode, AttributeCookie*& _cookie)
{
	if (package == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the attribute
	AutoPackageAttribute attribute;
	if (!AttributeForName(name, attribute))
		return B_ENTRY_NOT_FOUND;

	// allocate the cookie
	AutoPackageAttributeCookie* cookie = new(std::nothrow)
		AutoPackageAttributeCookie(package, attribute);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	_cookie = cookie;
	return B_OK;
}
