/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_CATEGORY_H
#define PACKAGE_CATEGORY_H


#include <Referenceable.h>
#include <String.h>


/*! This class represents a category into which a package may be assigned. The
	`code` corresponds to a code on the server.

    No builder is provided for this class; always create new instances using
    the constructor.
*/

class PackageCategory : public BReferenceable {
public:
								PackageCategory();
								PackageCategory(const BString& code,
									const BString& name);
								PackageCategory(const PackageCategory& other);

			bool				operator<(const PackageCategory& other) const;
			bool				operator==(const PackageCategory& other) const;
			bool				operator!=(const PackageCategory& other) const;

			const BString&		Code() const
									{ return fCode; }
			const BString&		Name() const
									{ return fName; }

			int					Compare(const PackageCategory& other) const;

private:
			BString				fCode;
			BString				fName;
};


typedef BReference<PackageCategory> CategoryRef;


extern bool IsPackageCategoryRefLess(const CategoryRef& c1, const CategoryRef& c2);


#endif // PACKAGE_CATEGORY_H
