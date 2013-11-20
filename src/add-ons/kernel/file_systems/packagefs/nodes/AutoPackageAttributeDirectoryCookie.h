/*
 * Copyright 2011-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef AUTO_PACKAGE_ATTRIBUTE_DIRECTORY_COOKIE_H
#define AUTO_PACKAGE_ATTRIBUTE_DIRECTORY_COOKIE_H


#include "AttributeDirectoryCookie.h"
#include "AutoPackageAttributes.h"


/*!
	When used for nodes that only have the automatic package attributes, the
	class can be used as is. Otherwise subclassing is required.
	Derived classes need to override Rewind() to rewind to their first
	attribute, but also call the base class version. Furthermore they must
	implement CurrentCustomAttributeName() to return the current attribute's
	name and NextCustomAttributeName() to iterate to the next attribute and
	return its name.
*/
class AutoPackageAttributeDirectoryCookie : public AttributeDirectoryCookie {
public:
								AutoPackageAttributeDirectoryCookie();
	virtual						~AutoPackageAttributeDirectoryCookie();

	virtual	status_t			Read(dev_t volumeID, ino_t nodeID,
									struct dirent* buffer, size_t bufferSize,
									uint32* _count);
	virtual	status_t			Rewind();

protected:
	virtual	String				CurrentCustomAttributeName();
	virtual	String				NextCustomAttributeName();

private:
			uint32				fState;
};


#endif	// AUTO_PACKAGE_ATTRIBUTE_DIRECTORY_COOKIE_H
