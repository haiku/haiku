/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_VERSION_H
#define PACKAGE_VERSION_H


#include <package/PackageInfo.h>

#include <Referenceable.h>
#include <String.h>

using BPackageKit::BPackageVersion;


class PackageVersion : public BPackageVersion, public BReferenceable
{
public:
								PackageVersion();
								PackageVersion(const PackageVersion& other);
								PackageVersion(const BPackageVersion& other);
	virtual						~PackageVersion();

			PackageVersion&		operator=(const PackageVersion& other);
			bool				operator==(const PackageVersion& other) const;
			bool				operator!=(const PackageVersion& other) const;

			uint64				CreateTimestamp() const
									{ return fCreateTimestamp; }
			void				SetCreateTimestamp(uint64 value);

private:
			uint64				fCreateTimestamp;
				// milliseconds since epoch
};


typedef BReference<PackageVersion> PackageVersionRef;


#endif // PACKAGE_VERSION_H
