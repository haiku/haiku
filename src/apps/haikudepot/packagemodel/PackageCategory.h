/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_CATEGORY_H
#define PACKAGE_CATEGORY_H


#include <Referenceable.h>
#include <String.h>


class PackageCategory : public BReferenceable {
public:
								PackageCategory();
								PackageCategory(const BString& code,
									const BString& name);
								PackageCategory(const PackageCategory& other);

			PackageCategory&	operator=(const PackageCategory& other);
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


extern bool IsPackageCategoryBefore(const CategoryRef& c1, const CategoryRef& c2);


#endif // PACKAGE_CATEGORY_H
