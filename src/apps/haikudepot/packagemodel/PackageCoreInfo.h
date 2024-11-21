/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_CORE_INFO_H
#define PACKAGE_CORE_INFO_H


#include <Referenceable.h>
#include <String.h>

#include "PackagePublisherInfo.h"
#include "PackageVersion.h"

using BPackageKit::BPackageVersion;


/*!	This class models the core data for the package.
 */
class PackageCoreInfo : public BReferenceable
{
public:
								PackageCoreInfo();
								PackageCoreInfo(const PackageCoreInfo& other);
	virtual						~PackageCoreInfo();

			PackageCoreInfo&	operator=(const PackageCoreInfo& other);
			bool				operator==(const PackageCoreInfo& other) const;
			bool				operator!=(const PackageCoreInfo& other) const;

			const PackageVersionRef&
								Version() const
									{ return fVersion; }
			void				SetVersion(PackageVersionRef value);

			const PackagePublisherInfoRef&
								Publisher() const
									{ return fPublisher; }
			void				SetPublisher(PackagePublisherInfoRef value);

			const BString		Architecture() const
									{ return fArchitecture; }
			void				SetArchitecture(const BString& value);

			void				SetDepotName(const BString& value);
			const BString&		DepotName() const
									{ return fDepotName; }

private:
			PackageVersionRef
								fVersion;
			PackagePublisherInfoRef
								fPublisher;
			BString				fArchitecture;
			BString				fDepotName;
};


typedef BReference<PackageCoreInfo> PackageCoreInfoRef;


#endif // PACKAGE_CORE_INFO_H
