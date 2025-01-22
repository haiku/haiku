/*
 * Copyright 2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_FILTER_SPECIFICATION_H
#define PACKAGE_FILTER_SPECIFICATION_H

#include <Referenceable.h>
#include <String.h>


/*!	Instances of this class should not be created directly; instead use the
	PackageFilterSpecificationBuilder class as a builder-constructor.
*/
class PackageFilterSpecification : public BReferenceable
{
friend class PackageFilterSpecificationBuilder;

public:
								PackageFilterSpecification();
	virtual						~PackageFilterSpecification();

			bool				operator==(const PackageFilterSpecification& other) const;
			bool				operator!=(const PackageFilterSpecification& other) const;

			BString				SearchTerms() const;
			BString				DepotName() const;
			BString				Category() const;
			bool				ShowAvailablePackages() const;
			bool				ShowInstalledPackages() const;
			bool				ShowSourcePackages() const;
			bool				ShowDevelopPackages() const;

private:
			void				SetSearchTerms(BString value);
			void				SetDepotName(BString value);
			void				SetCategory(BString value);
			void				SetShowAvailablePackages(bool value);
			void				SetShowInstalledPackages(bool value);
			void				SetShowSourcePackages(bool value);
			void				SetShowDevelopPackages(bool value);

private:
			BString				fSearchTerms;
			BString				fDepotName;
			BString				fCategory;
			bool				fShowAvailablePackages;
			bool				fShowInstalledPackages;
			bool				fShowSourcePackages;
			bool				fShowDevelopPackages;
};


typedef BReference<PackageFilterSpecification> PackageFilterSpecificationRef;


class PackageFilterSpecificationBuilder
{
public:
								PackageFilterSpecificationBuilder();
								PackageFilterSpecificationBuilder(
									const PackageFilterSpecificationRef& other);
	virtual						~PackageFilterSpecificationBuilder();

			PackageFilterSpecificationRef
								BuildRef();

			PackageFilterSpecificationBuilder
								WithSearchTerms(BString value);
			PackageFilterSpecificationBuilder
								WithDepotName(BString value);
			PackageFilterSpecificationBuilder
								WithCategory(BString value);
			PackageFilterSpecificationBuilder
								WithShowAvailablePackages(bool value);
			PackageFilterSpecificationBuilder
								WithShowInstalledPackages(bool value);
			PackageFilterSpecificationBuilder
								WithShowSourcePackages(bool value);
			PackageFilterSpecificationBuilder
								WithShowDevelopPackages(bool value);

private:
			void				_InitFromSource();
			void				_Init(const PackageFilterSpecification* value);

private:
			PackageFilterSpecificationRef
								fSource;
			BString				fSearchTerms;
			BString				fDepotName;
			BString				fCategory;
			bool				fShowAvailablePackages;
			bool				fShowInstalledPackages;
			bool				fShowSourcePackages;
			bool				fShowDevelopPackages;
};


#endif // PACKAGE_FILTER_SPECIFICATION_H
