/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_CORE_INFO_H
#define PACKAGE_CORE_INFO_H


#include <Referenceable.h>
#include <String.h>

#include "PackagePublisherInfo.h"
#include "PackageVersion.h"

using BPackageKit::BPackageVersion;


class PackageCoreInfoBuilder;


/*!	This class models the core data for the package.

	Instances of this class should not be created directly; instead use the
    PackageCoreInfoBuilder class as a builder-constructor.
 */
class PackageCoreInfo : public BReferenceable
{
friend class PackageCoreInfoBuilder;

public:
								PackageCoreInfo();
								PackageCoreInfo(const PackageCoreInfo& other);
	virtual						~PackageCoreInfo();

			bool				operator==(const PackageCoreInfo& other) const;
			bool				operator!=(const PackageCoreInfo& other) const;

			const PackageVersionRef&
								Version() const;
			const PackagePublisherInfoRef&
								Publisher() const;
			const BString		Architecture() const;
			const BString&		DepotName() const;

private:
			void				SetVersion(PackageVersionRef value);
			void				SetPublisher(PackagePublisherInfoRef value);
			void				SetArchitecture(const BString& value);
			void				SetDepotName(const BString& value);

private:
			PackageVersionRef
								fVersion;
			PackagePublisherInfoRef
								fPublisher;
			BString				fArchitecture;
			BString				fDepotName;
				// The name and not the identifier are used here because the
				// identifier is not readily available in all API situations.
};


typedef BReference<PackageCoreInfo> PackageCoreInfoRef;


class PackageCoreInfoBuilder
{
public:
								PackageCoreInfoBuilder();
								PackageCoreInfoBuilder(const PackageCoreInfoRef& other);
	virtual						~PackageCoreInfoBuilder();

			PackageCoreInfoRef	BuildRef();

			PackageCoreInfoBuilder&
								WithVersion(PackageVersionRef value);
			PackageCoreInfoBuilder&
								WithPublisher(PackagePublisherInfoRef value);
			PackageCoreInfoBuilder&
								WithArchitecture(const BString& value);
			PackageCoreInfoBuilder&
								WithDepotName(const BString& value);

private:
			void				_InitFromSource();
			void				_Init(const PackageCoreInfo* value);

private:
			PackageCoreInfoRef
								fSource;
			PackageVersionRef
								fVersion;
			PackagePublisherInfoRef
								fPublisher;
			BString				fArchitecture;
			BString				fDepotName;
};


#endif // PACKAGE_CORE_INFO_H
