/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_CLASSIFICATION_INFO_H
#define PACKAGE_CLASSIFICATION_INFO_H


#include <vector>

#include <Referenceable.h>

#include "PackageCategory.h"


class PackageClassificationInfoBuilder;


/*!	Instances of this class should not be created directly; instead use the
	PackageClassificationInfoBuilder class as a builder-constructor.
*/
class PackageClassificationInfo : public BReferenceable
{
friend class PackageClassificationInfoBuilder;

public:
								PackageClassificationInfo();
								PackageClassificationInfo(const PackageClassificationInfo& other);

			bool				operator==(const PackageClassificationInfo& other) const;
			bool				operator!=(const PackageClassificationInfo& other) const;

			uint32				Prominence() const;
			bool				HasProminence() const;
			bool				IsProminent() const;

			int32				CountCategories() const;
			const CategoryRef	CategoryAtIndex(int32 index) const;
			bool				HasCategoryByCode(const BString& code) const;

			bool				IsNativeDesktop() const;

private:
			void				SetProminence(uint32 prominence);
			void				ClearCategories();
			bool				AddCategory(const CategoryRef& category);
			void				SetIsNativeDesktop(bool value);

private:
			std::vector<CategoryRef>
								fCategories;
			uint32				fProminence;
			bool				fIsNativeDesktop;
};


typedef BReference<PackageClassificationInfo> PackageClassificationInfoRef;


class PackageClassificationInfoBuilder
{
public:
								PackageClassificationInfoBuilder();
								PackageClassificationInfoBuilder(
									const PackageClassificationInfoRef& value);
	virtual						~PackageClassificationInfoBuilder();

			PackageClassificationInfoRef
								BuildRef() const;

			PackageClassificationInfoBuilder&
								WithProminence(uint32 prominence);
			PackageClassificationInfoBuilder&
								WithIsNativeDesktop(bool value);

			PackageClassificationInfoBuilder&
								AddCategory(const CategoryRef& category);

private:
			void				_InitFromSource();
			void				_Init(const PackageClassificationInfo* value);

private:
			PackageClassificationInfoRef
								fSource;
			std::vector<CategoryRef>
								fCategories;
			uint32				fProminence;
			bool				fIsNativeDesktop;
};


#endif // PACKAGE_CLASSIFICATION_INFO_H
