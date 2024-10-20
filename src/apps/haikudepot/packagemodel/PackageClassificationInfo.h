/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_CLASSIFICATION_INFO_H
#define PACKAGE_CLASSIFICATION_INFO_H


#include <vector>

#include <Referenceable.h>

#include "PackageCategory.h"


class PackageClassificationInfo : public BReferenceable
{
public:
								PackageClassificationInfo();
								PackageClassificationInfo(const PackageClassificationInfo& other);

			bool				operator==(const PackageClassificationInfo& other) const;
			bool				operator!=(const PackageClassificationInfo& other) const;

			void				SetProminence(uint32 prominence);
			uint32				Prominence() const
									{ return fProminence; }
			bool				HasProminence() const
									{ return fProminence != 0; }
			bool				IsProminent() const;

			void				ClearCategories();
			bool				AddCategory(const CategoryRef& category);
			int32				CountCategories() const;
			CategoryRef			CategoryAtIndex(int32 index) const;
			bool				HasCategoryByCode(const BString& code) const;

			bool				IsNativeDesktop() const;
			void				SetIsNativeDesktop(bool value);

private:
			std::vector<CategoryRef>
								fCategories;
			uint32				fProminence;
			bool				fIsNativeDesktop;
};


typedef BReference<PackageClassificationInfo> PackageClassificationInfoRef;


#endif // PACKAGE_CLASSIFICATION_INFO_H
